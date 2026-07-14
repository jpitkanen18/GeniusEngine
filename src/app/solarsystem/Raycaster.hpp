#pragma once
// Omnidirectional Raycaster
// Casts rays radially outward from the sun across the orbital plane.
// Each ray is traced along a curved geodesic (gravitational lensing).
// When a ray hits a body, it records the hit point on that body's surface.
// Shadow = fraction of sun's light blocked by intervening bodies.
// Per-vertex shading: each vertex's illumination is computed from how many
// rays reached its region vs how many were blocked.

#include "../../engine/Mesh.hpp"
#include "../../engine/Renderer.hpp"
#include "CelestialBody.hpp"
#include <vector>
#include <cmath>
#include <omp.h>
#include <algorithm>

using namespace GE;

struct Raycaster
{
	// --- Configuration ---
	static constexpr int NUM_OMNI_RAYS = 2048; // omnidirectional rays from sun
	static constexpr int TARGETED_RAYS = 1536; // targeted rays per body (3 layers × 512)
	static constexpr int RAYS_PER_LAYER = 512; // rays per vertical layer
	static constexpr int NUM_LAYERS = 3;	   // vertical layers (top, middle, bottom)
	static constexpr int RAY_STEPS = 200;	   // integration steps per ray
	static constexpr float MAX_RANGE = 70.0f;  // how far omni rays travel
	static constexpr int MAX_VIS_RAYS = 64;	   // max visualized ray paths
	static constexpr int POINTS_PER_VIS_RAY = 50;
	static constexpr float LENSING_STRENGTH_DEFAULT = 8.0f;

	struct BodyIllumination
	{
		float shadowFactor = 0.0f;
		int primaryOccluder = -1;
		static constexpr int SHADOW_RES = 32;
		float shadowMap[SHADOW_RES] = {};
		int hitCount[SHADOW_RES] = {};
		int totalCount[SHADOW_RES] = {};
	};

	std::vector<BodyIllumination> bodyIllum;

	// Visualization
	bool showRays = false;
	float lensingScale = LENSING_STRENGTH_DEFAULT;
	Mesh rayMesh;
	std::vector<Vertex> rayVertices;
	int numVisPoints = 0;

	void init(size_t bodyCount)
	{
		bodyIllum.resize(bodyCount);
		rayMesh.createDynamic(MAX_VIS_RAYS * POINTS_PER_VIS_RAY);
		rayVertices.reserve(MAX_VIS_RAYS * POINTS_PER_VIS_RAY);
	}

	// Test if a ray segment intersects a body's ring disc.
	// Ring is an annulus in the XZ plane, tilted by ringTilt around X, centered at body position.
	// Returns true if the previous step to current pos crossed the ring plane within [innerR, outerR].
	static bool hitsRing(Vec3 prevPos, Vec3 curPos, const CelestialBody &body)
	{
		if (!body.hasRing)
			return false;

		// Ring normal: Y-axis rotated by ringTilt around X
		float tilt = glm::radians(body.ringTilt);
		Vec3 ringNormal = {0.0f, cosf(tilt), sinf(tilt)};

		Vec3 center = body.position;
		// Signed distances from ring plane
		float d0 = glm::dot(prevPos - center, ringNormal);
		float d1 = glm::dot(curPos - center, ringNormal);

		// Must cross the plane (sign change)
		if (d0 * d1 > 0.0f)
			return false;

		// Interpolate to find intersection point
		float t = d0 / (d0 - d1);
		Vec3 hit = prevPos + t * (curPos - prevPos);

		// Check if hit is within annulus
		float dist = glm::length(hit - center);
		return dist >= body.ringInnerRadius && dist <= body.ringOuterRadius;
	}

	void update(const std::vector<CelestialBody> &bodies)
	{
		int numBodies = (int)bodies.size();
		Vec3 sunPos = bodies[0].position;
		float sunRadius = bodies[0].radius;

		// Reset illumination
		for (int i = 0; i < numBodies; ++i)
		{
			bodyIllum[i].shadowFactor = 0.0f;
			bodyIllum[i].primaryOccluder = -1;
			for (int s = 0; s < BodyIllumination::SHADOW_RES; ++s)
			{
				bodyIllum[i].shadowMap[s] = 0.0f;
				bodyIllum[i].hitCount[s] = 0;
				bodyIllum[i].totalCount[s] = 0;
			}
		}

		// === PHASE 1: Targeted rays per body ===
		// 3-layer diamond pattern: rays emit from top, middle, and bottom of sun disk.
		// Each layer is offset by 60° in angle so they interleave (diamond pattern),
		// giving full coverage including vertical/polar regions.
		int targetedTotal = (numBodies - 1) * TARGETED_RAYS;

#pragma omp parallel
		{
			// Thread-local counters
			std::vector<std::vector<int>> localHit(numBodies, std::vector<int>(BodyIllumination::SHADOW_RES, 0));
			std::vector<std::vector<int>> localTotal(numBodies, std::vector<int>(BodyIllumination::SHADOW_RES, 0));
			std::vector<int> localOccluder(numBodies, -1);
			std::vector<int> localOccCount(numBodies * numBodies, 0);

#pragma omp for schedule(static)
			for (int rayIdx = 0; rayIdx < targetedTotal; ++rayIdx)
			{
				int targetBody = (rayIdx / TARGETED_RAYS) + 1;
				int localIdx = rayIdx % TARGETED_RAYS;
				int layer = localIdx / RAYS_PER_LAYER; // 0, 1, 2
				int sampleIdx = localIdx % RAYS_PER_LAYER;

				Vec3 toTarget = glm::normalize(bodies[targetBody].position - sunPos);
				Vec3 upS = (std::abs(toTarget.y) < 0.99f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
				Vec3 rightS = glm::normalize(glm::cross(toTarget, upS));
				Vec3 diskUpS = glm::cross(rightS, toTarget);

				// Sun disk origin: 3 vertical layers in world-space Y
				// Layer 0 = top, Layer 1 = middle, Layer 2 = bottom
				float layerY = (1 - layer) * 0.7f * sunRadius; // +0.7, 0, -0.7 in world Y
				// Angular offset per layer for diamond interleave: 0°, 120°, 240°
				float angularPhase = layer * (2.0f * glm::pi<float>() / 3.0f);

				float sunAngle = 2.0f * glm::pi<float>() * (float)(sampleIdx % 64) / 64.0f + angularPhase;
				float sunR = (float)((sampleIdx / 64) + 1) / (float)(RAYS_PER_LAYER / 64 + 1);
				Vec3 rayOrigin = sunPos + (rightS * std::cos(sunAngle) + diskUpS * std::sin(sunAngle)) * sunR * sunRadius + Vec3(0, layerY, 0); // vertical offset in world space

				// Target: each layer aims at a different latitude band on the planet
				// Layer 0 → north pole, Layer 1 → equator, Layer 2 → south pole
				float tgtAngle = 2.0f * glm::pi<float>() * (float)(sampleIdx % 32) / 32.0f + angularPhase;
				float tgtR = (float)((sampleIdx % 8) + 1) / 9.0f * 0.8f;

				Vec3 toSun = glm::normalize(sunPos - bodies[targetBody].position);
				Vec3 upT = (std::abs(toSun.y) < 0.99f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
				Vec3 rightT = glm::normalize(glm::cross(toSun, upT));
				Vec3 diskUpT = glm::cross(rightT, toSun);

				// Latitude: layer 0 targets north (+Y), layer 2 targets south (-Y)
				// This uses world Y directly mixed with the disk basis
				float polarBias = (1 - layer) * 0.6f; // +0.6, 0, -0.6
				float tgtX = tgtR * std::cos(tgtAngle);
				float tgtYcomp = tgtR * std::sin(tgtAngle) + polarBias;

				// Build target point on sphere surface
				// Use world Y for polar targeting so rays actually hit poles
				Vec3 localDir = rightT * tgtX + diskUpT * tgtYcomp + toSun * 0.5f;
				Vec3 targetPoint = bodies[targetBody].position +
								   glm::normalize(localDir) * bodies[targetBody].radius;

				// Trace ray
				Vec3 dir = glm::normalize(targetPoint - rayOrigin);
				float totalDist = glm::length(targetPoint - rayOrigin);
				float stepSize = totalDist / (float)RAY_STEPS;
				Vec3 pos = rayOrigin;
				Vec3 vel = dir;

				int sector = computeSector(targetPoint, bodies[targetBody].position, sunPos);
				localTotal[targetBody][sector]++;

				bool occluded = false;
				int occluder = -1;
				for (int step = 0; step < RAY_STEPS; ++step)
				{
					// Gravitational lensing
					Vec3 deflection(0.0f);
					for (int b = 1; b < numBodies; ++b)
					{
						if (b == targetBody)
							continue;
						Vec3 toBody = bodies[b].position - pos;
						float dist2 = glm::dot(toBody, toBody);
						float dist = std::sqrt(dist2);
						if (dist < bodies[b].radius * 0.3f)
							continue;
						float force = lensingScale * bodies[b].mass / (dist2 + 1.0f);
						Vec3 forceDir = toBody / dist;
						float proj = glm::dot(forceDir, vel);
						Vec3 perp = forceDir - proj * vel;
						deflection += perp * force * stepSize * 0.001f;
					}
					vel = glm::normalize(vel + deflection);
					Vec3 prevPos = pos;
					pos += vel * stepSize;

					// Check occlusion by intermediate bodies and rings
					for (int b = 1; b < numBodies; ++b)
					{
						if (b == targetBody)
							continue;
						float d = glm::length(pos - bodies[b].position);
						if (d < bodies[b].radius * 1.2f)
						{
							occluded = true;
							occluder = b;
							goto doneTargeted;
						}
						// Ring occlusion
						if (hitsRing(prevPos, pos, bodies[b]))
						{
							occluded = true;
							occluder = b;
							goto doneTargeted;
						}
					}

					// Check if target body's own ring shadows itself
					if (hitsRing(prevPos, pos, bodies[targetBody]))
					{
						occluded = true;
						occluder = targetBody;
						goto doneTargeted;
					}

					// Reached target?
					if (glm::length(pos - targetPoint) < bodies[targetBody].radius * 0.4f)
						goto doneTargeted;
				}
			doneTargeted:

				if (occluded)
				{
					localHit[targetBody][sector]++;
					if (occluder > 0)
						localOccCount[targetBody * numBodies + occluder]++;
				}
			}

#pragma omp critical
			{
				for (int b = 1; b < numBodies; ++b)
				{
					for (int s = 0; s < BodyIllumination::SHADOW_RES; ++s)
					{
						bodyIllum[b].hitCount[s] += localHit[b][s];
						bodyIllum[b].totalCount[s] += localTotal[b][s];
					}
				}
				for (int i = 0; i < numBodies * numBodies; ++i)
				{
					if (localOccCount[i] > 0)
					{
						int target = i / numBodies;
						int occ = i % numBodies;
						// Track highest-count occluder
						if (localOccCount[i] > 0)
							bodyIllum[target].primaryOccluder = occ;
					}
				}
			}
		}

		// Compute per-body shadow factors and shadow maps
		for (int b = 1; b < numBodies; ++b)
		{
			auto &illum = bodyIllum[b];
			int totalHits = 0, totalRaysHit = 0;
			for (int s = 0; s < BodyIllumination::SHADOW_RES; ++s)
			{
				if (illum.totalCount[s] > 0)
				{
					illum.shadowMap[s] = (float)illum.hitCount[s] / (float)illum.totalCount[s];
					totalHits += illum.hitCount[s];
					totalRaysHit += illum.totalCount[s];
				}
			}
			illum.shadowFactor = (totalRaysHit > 0) ? (float)totalHits / (float)totalRaysHit : 0.0f;
		}

		if (showRays)
			buildVisualization(bodies);
	}

	int computeSector(Vec3 hitPos, Vec3 bodyPos, Vec3 sunPos) const
	{
		Vec3 toSun = glm::normalize(sunPos - bodyPos);
		Vec3 hitDir = glm::normalize(hitPos - bodyPos);
		Vec3 projOnPlane = hitDir - glm::dot(hitDir, toSun) * toSun;
		float projLen = glm::length(projOnPlane);
		if (projLen < 0.001f)
			return 0;

		Vec3 up(0, 1, 0);
		Vec3 right = glm::normalize(glm::cross(toSun, up));
		if (glm::length(right) < 0.001f)
			right = glm::normalize(glm::cross(toSun, Vec3(1, 0, 0)));
		Vec3 diskUp = glm::cross(right, toSun);

		float sx = glm::dot(projOnPlane, right);
		float sy = glm::dot(projOnPlane, diskUp);
		float sAngle = std::atan2(sy, sx) + glm::pi<float>();
		int sector = (int)(sAngle / (2.0f * glm::pi<float>()) * BodyIllumination::SHADOW_RES);
		return std::clamp(sector, 0, BodyIllumination::SHADOW_RES - 1);
	}

	// Get shadow at a surface point on body b
	float getShadowAt(int bodyIdx, Vec3 surfaceNormal, Vec3 sunDir) const
	{
		if (bodyIdx <= 0 || bodyIdx >= (int)bodyIllum.size())
			return 0.0f;

		const auto &illum = bodyIllum[bodyIdx];
		if (illum.shadowFactor < 0.001f)
			return 0.0f;

		// Project normal onto the plane perpendicular to sunDir
		Vec3 projOnPlane = surfaceNormal - glm::dot(surfaceNormal, sunDir) * sunDir;
		float projLen = glm::length(projOnPlane);

		// Face away from sun -> no shadow effect (dark side anyway)
		float sunDot = glm::dot(surfaceNormal, sunDir);
		if (sunDot < -0.1f)
			return 0.0f;

		if (projLen < 0.001f)
		{
			// Directly facing sun — average nearby sectors
			float avg = 0.0f;
			for (int s = 0; s < BodyIllumination::SHADOW_RES; ++s)
				avg += illum.shadowMap[s];
			return avg / BodyIllumination::SHADOW_RES;
		}

		Vec3 up(0, 1, 0);
		Vec3 right = glm::normalize(glm::cross(sunDir, up));
		if (glm::length(right) < 0.001f)
			right = glm::normalize(glm::cross(sunDir, Vec3(1, 0, 0)));
		Vec3 diskUp = glm::cross(right, sunDir);

		float sx = glm::dot(projOnPlane, right);
		float sy = glm::dot(projOnPlane, diskUp);
		float angle = std::atan2(sy, sx) + glm::pi<float>();

		// Bilinear interpolation between adjacent sectors
		float fSector = angle / (2.0f * glm::pi<float>()) * BodyIllumination::SHADOW_RES;
		int s0 = (int)fSector % BodyIllumination::SHADOW_RES;
		int s1 = (s0 + 1) % BodyIllumination::SHADOW_RES;
		float frac = fSector - (int)fSector;

		float shadow = illum.shadowMap[s0] * (1.0f - frac) + illum.shadowMap[s1] * frac;

		// Attenuate by how much this vertex faces the sun (penumbra near terminator)
		float faceFactor = std::clamp(sunDot * 3.0f + 0.5f, 0.0f, 1.0f);
		return shadow * faceFactor;
	}

	// Compute full path-traced illumination for all bodies.
	// The raycaster IS the lighting system — vertex colors carry final illumination.
	// For each vertex: diffuse from sun direction + eclipse shadow from raycaster + ring shadows.
	void applyLighting(std::vector<CelestialBody> &bodies,
					   std::vector<std::vector<Vertex>> &baseVertices)
	{
		Vec3 sunPos = bodies[0].position;
		std::vector<bool> needsGPUUpdate(bodies.size(), false);

#pragma omp parallel for schedule(dynamic)
		for (int i = 1; i < (int)bodies.size(); ++i)
		{
			Vec3 toSun = glm::normalize(sunPos - bodies[i].position);
			float sunDist = glm::length(sunPos - bodies[i].position);
			auto &bv = baseVertices[i];

			// Distance-based ambient — further from sun = dimmer ambient
			float distFalloff = std::clamp(8.0f / (sunDist + 1.0f), 0.05f, 1.0f);

			for (size_t vi = 0; vi < bv.size(); ++vi)
			{
				Vec3 n = glm::normalize(bv[vi].normal);

				// --- Diffuse illumination from sun ---
				float NdotL = glm::dot(n, toSun);
				float diffuse = std::max(0.0f, NdotL);

				// Softer terminator (wrap lighting)
				float wrap = std::clamp((NdotL + 0.15f) / 1.15f, 0.0f, 1.0f);

				// --- Eclipse shadow from raycaster ---
				float eclipse = getShadowAt(i, n, toSun);

				// --- Ring shadow on planet ---
				float ringShadow = 0.0f;
				if (bodies[i].hasRing)
				{
					// Check if this vertex is shadowed by the ring
					// Vertex world position = body position + normal * radius (approximately)
					Vec3 vertWorld = bv[vi].position; // local space, relative to body center
					// Cast a ray from vertex toward sun, check ring intersection
					Vec3 vertToSun = toSun;
					// Check if line from vertex to sun crosses ring plane
					float tilt = glm::radians(bodies[i].ringTilt);
					Vec3 ringNormal = {0.0f, cosf(tilt), sinf(tilt)};
					float denom = glm::dot(vertToSun, ringNormal);
					if (std::abs(denom) > 0.001f)
					{
						float t = -glm::dot(vertWorld, ringNormal) / denom;
						if (t > 0.0f) // Ring plane is between vertex and sun
						{
							Vec3 hitLocal = vertWorld + vertToSun * t;
							float hitDist = glm::length(hitLocal);
							if (hitDist >= bodies[i].ringInnerRadius && hitDist <= bodies[i].ringOuterRadius)
							{
								// In ring shadow — opacity varies with radial position
								float ringT = (hitDist - bodies[i].ringInnerRadius) /
											  (bodies[i].ringOuterRadius - bodies[i].ringInnerRadius);
								ringShadow = 0.6f * (1.0f - ringT * 0.5f); // inner ring denser
							}
						}
					}
				}

				// --- Combine ---
				float ambient = 0.06f * distFalloff;
				float lit = ambient + wrap * distFalloff * (1.0f - eclipse * 0.9f) * (1.0f - ringShadow);
				lit = std::clamp(lit, 0.0f, 1.0f);

				Color c = bodies[i].color * lit;

				// Eclipse tint — reddish penumbra
				if (eclipse > 0.1f)
					c.r = std::min(1.0f, c.r + eclipse * 0.08f * distFalloff);

				c.a = 1.0f;
				bv[vi].color = c;
			}
			needsGPUUpdate[i] = true;
		}

		// Upload on main thread
		for (int i = 1; i < (int)bodies.size(); ++i)
		{
			if (needsGPUUpdate[i])
				bodies[i].sphereMesh.updateVertices(baseVertices[i]);
		}

		// --- Ring lighting: sun illumination + planet body shadow + eclipse shadow ---
		for (size_t bi = 0; bi < bodies.size(); ++bi)
		{
			auto &body = bodies[bi];
			if (!body.hasRing || body.ringMesh.getVertices().empty())
				continue;

			Vec3 toSun = glm::normalize(sunPos - body.position);
			float tilt = glm::radians(body.ringTilt);
			auto &ringVerts = body.ringMesh.getVertices();
			auto ringCopy = ringVerts;

			for (size_t vi = 0; vi < ringCopy.size(); ++vi)
			{
				Vec3 localPos = body.ringBaseVertices.empty()
									? ringVerts[vi].position
									: body.ringBaseVertices[vi].position;

				// Rotate ring vertex to tilted frame
				Vec3 worldRingPos;
				worldRingPos.x = localPos.x;
				worldRingPos.y = localPos.y * cosf(tilt) - localPos.z * sinf(tilt);
				worldRingPos.z = localPos.y * sinf(tilt) + localPos.z * cosf(tilt);

				// Sun illumination on ring — both sides lit
				Vec3 ringNormal = {0.0f, cosf(tilt), sinf(tilt)};
				float NdotL = std::abs(glm::dot(ringNormal, toSun));
				float ringLit = 0.15f + 0.85f * NdotL;

				// Planet body shadow on ring
				float projOnSun = glm::dot(worldRingPos, toSun);
				if (projOnSun < 0.0f)
				{
					Vec3 perpFromAxis = worldRingPos - projOnSun * toSun;
					float perpDist = glm::length(perpFromAxis);
					if (perpDist < body.radius)
					{
						float edgeFade = perpDist / body.radius;
						float shadow = 1.0f - edgeFade * edgeFade;
						ringLit *= (1.0f - shadow * 0.85f);
					}
				}

				// Eclipse shadow on ring (from other bodies occluding sunlight)
				// Use the ring vertex direction from body center as a "surface normal" for sector lookup
				float ringLen = glm::length(worldRingPos);
				if (ringLen > 0.001f && (int)bi > 0)
				{
					Vec3 ringDir = worldRingPos / ringLen;
					float eclipse = getShadowAt((int)bi, ringDir, toSun);
					ringLit *= (1.0f - eclipse * 0.9f);
				}

				Color baseColor = body.ringBaseVertices.empty()
									  ? ringVerts[vi].color
									  : body.ringBaseVertices[vi].color;
				ringCopy[vi].color = Color(baseColor.r * ringLit, baseColor.g * ringLit,
										   baseColor.b * ringLit, baseColor.a);
			}

			body.ringMesh.updateVertices(ringCopy);
		}
	}

	void buildVisualization(const std::vector<CelestialBody> &bodies)
	{
		rayVertices.clear();
		Vec3 sunPos = bodies[0].position;
		float sunRadius = bodies[0].radius;
		int numBodies = (int)bodies.size();

		int raysDrawn = 0;

		// Visualize actual targeted rays from the 3-layer diamond pattern
		// Show a subset across all layers and targets
		int raysPerVis = MAX_VIS_RAYS / NUM_LAYERS; // ~21 per layer
		int targetStep = std::max(1, (numBodies - 1));
		int sampleStep = std::max(1, RAYS_PER_LAYER / (raysPerVis / std::max(1, numBodies - 1)));

		for (int layer = 0; layer < NUM_LAYERS && raysDrawn < MAX_VIS_RAYS; ++layer)
		{
			float layerY = (1 - layer) * 0.7f * sunRadius;
			float angularPhase = layer * (2.0f * glm::pi<float>() / 3.0f);

			for (int tb = 1; tb < numBodies && raysDrawn < MAX_VIS_RAYS; ++tb)
			{
				Vec3 toTarget = glm::normalize(bodies[tb].position - sunPos);
				Vec3 upS = (std::abs(toTarget.y) < 0.99f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
				Vec3 rightS = glm::normalize(glm::cross(toTarget, upS));
				Vec3 diskUpS = glm::cross(rightS, toTarget);

				// Show a few rays per layer per target
				int visPerTarget = std::max(1, MAX_VIS_RAYS / (NUM_LAYERS * (numBodies - 1)));
				int visStep = std::max(1, RAYS_PER_LAYER / visPerTarget);

				for (int si = 0; si < RAYS_PER_LAYER && raysDrawn < MAX_VIS_RAYS; si += visStep)
				{
					float sunAngle = 2.0f * glm::pi<float>() * (float)(si % 64) / 64.0f + angularPhase;
					float sunR = (float)((si / 64) + 1) / (float)(RAYS_PER_LAYER / 64 + 1);
					Vec3 rayOrigin = sunPos + (rightS * std::cos(sunAngle) + diskUpS * std::sin(sunAngle)) * sunR * sunRadius + Vec3(0, layerY, 0);

					// Target point (same as actual ray computation)
					float tgtAngle = 2.0f * glm::pi<float>() * (float)(si % 32) / 32.0f + angularPhase;
					float tgtR = (float)((si % 8) + 1) / 9.0f * 0.8f;
					Vec3 toSun = glm::normalize(sunPos - bodies[tb].position);
					Vec3 upT = (std::abs(toSun.y) < 0.99f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
					Vec3 rightT = glm::normalize(glm::cross(toSun, upT));
					Vec3 diskUpT = glm::cross(rightT, toSun);
					float polarBias = (1 - layer) * 0.6f;
					float tgtX = tgtR * std::cos(tgtAngle);
					float tgtYcomp = tgtR * std::sin(tgtAngle) + polarBias;
					Vec3 localDir = rightT * tgtX + diskUpT * tgtYcomp + toSun * 0.5f;
					Vec3 targetPoint = bodies[tb].position + glm::normalize(localDir) * bodies[tb].radius;

					// Trace the ray with lensing
					Vec3 dir = glm::normalize(targetPoint - rayOrigin);
					float totalDist = glm::length(targetPoint - rayOrigin);
					float stepSize = totalDist / (float)RAY_STEPS;
					Vec3 pos = rayOrigin;
					Vec3 vel = dir;

					bool hitSomething = false;
					std::vector<Vec3> path;
					path.push_back(pos);

					for (int s = 0; s < RAY_STEPS; ++s)
					{
						Vec3 deflection(0.0f);
						for (int b = 1; b < numBodies; ++b)
						{
							if (b == tb)
								continue;
							Vec3 toBody = bodies[b].position - pos;
							float dist2 = glm::dot(toBody, toBody);
							float dist = std::sqrt(dist2);
							if (dist < bodies[b].radius * 0.3f)
								continue;
							float force = lensingScale * bodies[b].mass / (dist2 + 1.0f);
							Vec3 forceDir = toBody / dist;
							float proj = glm::dot(forceDir, vel);
							Vec3 perp = forceDir - proj * vel;
							deflection += perp * force * stepSize * 0.001f;
						}
						vel = glm::normalize(vel + deflection);
						pos += vel * stepSize;

						if (s % 4 == 0)
							path.push_back(pos);

						for (int b = 1; b < numBodies; ++b)
						{
							if (b == tb)
								continue;
							if (glm::length(pos - bodies[b].position) < bodies[b].radius * 1.1f)
							{
								hitSomething = true;
								path.push_back(pos);
								goto doneVis;
							}
						}
						if (glm::length(pos - targetPoint) < bodies[tb].radius * 0.5f)
						{
							path.push_back(pos);
							goto doneVis;
						}
					}
				doneVis:

					if (path.size() < 2)
						continue;

					// Color by layer: top=cyan, mid=yellow, bottom=magenta
					Color layerColors[3] = {
						{0.3f, 1.0f, 1.0f, 0.3f}, // top - cyan
						{1.0f, 1.0f, 0.3f, 0.3f}, // mid - yellow
						{1.0f, 0.3f, 1.0f, 0.3f}  // bottom - magenta
					};
					Color rayColor = hitSomething
										 ? Color{1.0f, 0.3f, 0.1f, 0.5f}
										 : layerColors[layer];

					for (size_t p = 0; p < path.size(); ++p)
					{
						Vertex v;
						v.position = path[p];
						v.normal = {0, 1, 0};
						v.color = rayColor;
						v.color.a *= 0.3f + 0.7f * (float)p / (float)path.size();
						rayVertices.push_back(v);
					}
					// Separator vertex
					if (raysDrawn < MAX_VIS_RAYS - 1)
					{
						Vertex sep;
						sep.position = path.back();
						sep.normal = {0, 1, 0};
						sep.color = {0, 0, 0, 0};
						rayVertices.push_back(sep);
					}
					++raysDrawn;
				}
			}
		}

		numVisPoints = (int)rayVertices.size();
		if (numVisPoints > 0)
		{
			if (numVisPoints > MAX_VIS_RAYS * POINTS_PER_VIS_RAY)
				rayVertices.resize(MAX_VIS_RAYS * POINTS_PER_VIS_RAY);
			rayMesh.updateVertices(rayVertices);
		}
	}

	void draw(Renderer &renderer, Graphics::IShader *lineShader)
	{
		if (!showRays || numVisPoints < 2)
			return;
		Transform t;
		renderer.drawMeshSimple(rayMesh, lineShader, t);
	}
};
