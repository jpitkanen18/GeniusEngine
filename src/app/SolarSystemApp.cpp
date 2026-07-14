// GeniusEngine - Solar System & Spacetime Simulation
// Demonstrates: orbital mechanics, spacetime curvature, asteroid belt,
// omnidirectional relativistic raycasting, per-vertex eclipse shadows.

#include "../engine/Application.hpp"
#include "./solarsystem/CelestialBody.hpp"
#include "./solarsystem/SpacetimeGrid.hpp"
#include "./solarsystem/TrailRenderer.hpp"
#include "./solarsystem/AsteroidBelt.hpp"
#include "./solarsystem/Raycaster.hpp"
#include "./solarsystem/SolarSystemPanel.hpp"
#include "../graphics/Factory.hpp"

#include <omp.h>

using namespace GE;

class SolarSystemApp : public GE::Application
{
public:
	void configure(EngineConfig &config) override
	{
		config.window.width = 1600;
		config.window.height = 900;
		config.window.title = "GeniusEngine - Solar System & Spacetime";
		config.window.backend = Graphics::Backend::Metal;
	}

	void onInit() override
	{
		this->m_defaultShader = renderer().createDefaultShader();
		this->m_lineShader = renderer().createLineShader();
		this->m_unlitShader = renderer().createUnlitShader();

		Light sunLight;
		sunLight.position = {0, 0, 0};
		sunLight.color = {1.0f, 0.95f, 0.8f, 1.0f};
		sunLight.intensity = 2.0f;
		renderer().setLight(sunLight);

		initBodies();
		initTrails();
		initOrbits();

		this->m_spacetime.init();
		this->m_asteroidBelt.init(this->m_defaultShader);
		this->m_raycaster.init(this->m_bodies.size());

		// Populate panel device info once
		auto *ctx = window().getContext();
		this->m_panel.backendName = Graphics::backendName(Graphics::Factory::getBackend());
		this->m_panel.deviceName = ctx->getDeviceName();
		this->m_panel.apiVersion = ctx->getAPIVersion();
		this->m_panel.driverVersion = ctx->getDriverVersion();
	}

	void onInput(float dt) override
	{
		auto &in = input();

		if (in.isKeyDown(Key::Escape))
			in.requestClose();

		// Toggle keys (edge-triggered)
		if (in.isKeyPressed(Key::Space))
			this->m_paused = !this->m_paused;
		if (in.isKeyPressed(Key::Tab))
			this->m_showUI = !this->m_showUI;

		// --- WASD + QE + RF free-fly ---
		float moveSpeed = 30.0f * dt;
		bool moved = false;
		if (in.isKeyDown(Key::W))
		{
			camera().moveForward(moveSpeed);
			moved = true;
		}
		if (in.isKeyDown(Key::S))
		{
			camera().moveForward(-moveSpeed);
			moved = true;
		}
		if (in.isKeyDown(Key::A))
		{
			camera().moveRight(-moveSpeed);
			moved = true;
		}
		if (in.isKeyDown(Key::D))
		{
			camera().moveRight(moveSpeed);
			moved = true;
		}
		if (in.isKeyDown(Key::R))
		{
			camera().moveUp(moveSpeed);
			moved = true;
		}
		if (in.isKeyDown(Key::F))
		{
			camera().moveUp(-moveSpeed);
			moved = true;
		}
		if (in.isKeyDown(Key::Q))
		{
			camera().rotateYaw(-1.5f * dt);
			moved = true;
		}
		if (in.isKeyDown(Key::E))
		{
			camera().rotateYaw(1.5f * dt);
			moved = true;
		}

		if (moved)
			this->m_followIdx = -1;

		// Number keys 0-9 to follow bodies
		for (int k = 0; k <= 9; ++k)
		{
			if (in.isKeyPressed((Key)((int)Key::Num0 + k)) && k < (int)this->m_bodies.size())
				this->m_followIdx = k;
		}

		// Scroll zoom
		float scroll = in.getScrollDelta();
		if (scroll != 0.0f)
			camera().orbit(0, 0, -scroll * 3.0f);

		// Mouse
		Vec2 mouseDelta = in.getMouseDelta();
		Vec2 mousePos = in.getMousePosition();

		if (in.isMouseDown(MouseButton::Left))
		{
			if (in.isMousePressed(MouseButton::Left))
			{
				this->m_clickStart = mousePos;
			}

			if (this->m_followIdx >= 0)
				camera().orbit(mouseDelta.x * 0.005f, mouseDelta.y * 0.005f, 0);
			else
				camera().mouseLook(mouseDelta.x * 0.003f, mouseDelta.y * 0.003f);
		}

		// LMB release — click detection for body picking
		if (in.isMouseReleased(MouseButton::Left))
		{
			float dragDist = glm::length(mousePos - this->m_clickStart);
			if (dragDist < 5.0f)
			{
				Ray ray = camera().screenToRay(mousePos.x, mousePos.y,
											   (float)window().getWidth(), (float)window().getHeight());
				int picked = pickBody(ray);
				if (picked >= 0)
					this->m_followIdx = picked;
			}
		}

		// Right mouse: always orbit
		if (in.isMouseDown(MouseButton::Right))
			camera().orbit(mouseDelta.x * 0.005f, mouseDelta.y * 0.005f, 0);
	}

	void onUpdate(float dt) override
	{
		// Camera follow
		if (this->m_followIdx >= 0 && this->m_followIdx < (int)this->m_bodies.size())
			camera().follow(this->m_bodies[this->m_followIdx].position);

		if (this->m_paused)
			return;

		for (size_t i = 0; i < this->m_bodies.size(); ++i)
		{
			auto &body = this->m_bodies[i];
			if (body.orbitalPeriod <= 0.0f)
				continue;

			body.angle += (2.0f * glm::pi<float>() / body.orbitalPeriod) * dt * this->m_timeScale;
			float incRad = glm::radians(body.inclination);
			body.position.x = body.orbitRadius * cos(body.angle);
			body.position.z = body.orbitRadius * sin(body.angle);
			body.position.y = body.orbitRadius * sin(body.angle) * sin(incRad);

			body.trail.push_back(body.position);
			if (body.trail.size() > TrailRenderer::MAX_TRAIL_POINTS)
				body.trail.pop_front();
		}

		if (this->m_showSpacetime)
			this->m_spacetime.update(this->m_bodies);

		this->m_asteroidBelt.update(dt, this->m_timeScale);

		if (this->m_showEclipses)
			this->m_raycaster.update(this->m_bodies);

		// All lighting is path-traced — always apply
		this->m_raycaster.applyLighting(this->m_bodies, this->m_baseVertices);
	}

	void onRender() override
	{
		if (this->m_showSpacetime)
		{
			Transform gridT;
			renderer().drawMeshSimple(this->m_spacetime.mesh, this->m_lineShader.get(), gridT);
		}

		if (this->m_showOrbits)
		{
			Transform orbitT;
			for (auto &circle : this->m_orbitCircles)
				renderer().drawMeshSimple(circle, this->m_lineShader.get(), orbitT);
		}

		for (auto &body : this->m_bodies)
		{
			Transform t;
			t.position = body.position;
			renderer().drawMesh(body.sphereMesh, body.material, t);

			if (body.hasRing)
			{
				Transform ringT;
				ringT.position = body.position;
				ringT.rotation = glm::angleAxis(glm::radians(body.ringTilt), Vec3(1, 0, 0));
				renderer().drawMesh(body.ringMesh, body.ringMaterial, ringT);
			}
		}

		this->m_asteroidBelt.draw(renderer());

		if (this->m_showTrails)
		{
			for (size_t i = 1; i < this->m_bodies.size(); ++i)
			{
				if (this->m_bodies[i].trail.size() > 2)
				{
					this->m_trails[i].update(this->m_bodies[i].trail, this->m_bodies[i].color);
					Transform trailT;
					renderer().drawMeshSimple(this->m_trails[i].mesh, this->m_lineShader.get(), trailT);
				}
			}
		}

		this->m_raycaster.draw(renderer(), this->m_lineShader.get());
	}

	bool wantsUI() override { return this->m_showUI; }

	void onUI() override
	{
		// Bind live data to the panel and draw
		this->m_panel.bodies = &this->m_bodies;
		this->m_panel.raycaster = &this->m_raycaster;
		this->m_panel.asteroidBelt = &this->m_asteroidBelt;
		this->m_panel.spacetime = &this->m_spacetime;
		this->m_panel.cam = &camera();
		this->m_panel.followIdx = &this->m_followIdx;
		this->m_panel.timeScale = &this->m_timeScale;
		this->m_panel.paused = &this->m_paused;
		this->m_panel.showOrbits = &this->m_showOrbits;
		this->m_panel.showTrails = &this->m_showTrails;
		this->m_panel.showSpacetime = &this->m_showSpacetime;
		this->m_panel.showEclipses = &this->m_showEclipses;
		float dt = deltaTime();
		this->m_panel.dt = &dt;
		this->m_panel.draw(ui());
	}

private:
	// Ray-sphere intersection for body picking
	int pickBody(const Ray &ray)
	{
		int bestIdx = -1;
		float bestDist = 1e30f;

		for (size_t i = 0; i < this->m_bodies.size(); ++i)
		{
			Vec3 oc = ray.origin - this->m_bodies[i].position;
			// Use a generous pick radius (2x visual radius for easier clicking)
			float r = this->m_bodies[i].radius * 2.0f;
			float a = glm::dot(ray.direction, ray.direction);
			float b = 2.0f * glm::dot(oc, ray.direction);
			float c = glm::dot(oc, oc) - r * r;
			float disc = b * b - 4.0f * a * c;
			if (disc < 0.0f)
				continue;
			float t = (-b - std::sqrt(disc)) / (2.0f * a);
			if (t < 0.0f)
				t = (-b + std::sqrt(disc)) / (2.0f * a);
			if (t > 0.0f && t < bestDist)
			{
				bestDist = t;
				bestIdx = (int)i;
			}
		}
		return bestIdx;
	}

	void initBodies()
	{
		this->m_bodies.push_back({"Sun", 0.0f, 2.0f, 0.0f, 0.0f, {1.0f, 0.9f, 0.3f, 1.0f}, 100.0f});
		this->m_bodies.push_back({"Mercury", 3.9f, 0.35f, 8.0f, 7.0f, {0.7f, 0.7f, 0.7f, 1.0f}, 0.06f});
		this->m_bodies.push_back({"Venus", 7.2f, 0.87f, 15.0f, 3.4f, {0.9f, 0.7f, 0.3f, 1.0f}, 0.82f});
		this->m_bodies.push_back({"Earth", 10.0f, 0.92f, 20.0f, 0.0f, {0.2f, 0.5f, 0.9f, 1.0f}, 1.0f});
		this->m_bodies.push_back({"Mars", 15.2f, 0.49f, 30.0f, 1.9f, {0.9f, 0.3f, 0.1f, 1.0f}, 0.11f});
		this->m_bodies.push_back({"Jupiter", 28.0f, 1.6f, 60.0f, 1.3f, {0.8f, 0.6f, 0.4f, 1.0f}, 30.0f});
		this->m_bodies.push_back({"Saturn", 38.0f, 1.35f, 90.0f, 2.5f, {0.9f, 0.8f, 0.5f, 1.0f}, 15.0f});
		this->m_bodies.push_back({"Uranus", 48.0f, 1.0f, 140.0f, 0.8f, {0.5f, 0.8f, 0.9f, 1.0f}, 4.0f});
		this->m_bodies.push_back({"Neptune", 56.0f, 0.95f, 200.0f, 1.8f, {0.2f, 0.3f, 0.9f, 1.0f}, 3.5f});

		this->m_baseVertices.resize(this->m_bodies.size());
		for (size_t bi = 0; bi < this->m_bodies.size(); ++bi)
		{
			auto &body = this->m_bodies[bi];
			int detail = (body.radius > 1.5f) ? 48 : 24;
			body.sphereMesh = Mesh::generateSphere(body.radius, detail, detail, body.color,
												   Graphics::BufferUsage::Dynamic);
			// All lighting comes from the raycaster — use unlit shader for everything
			body.material.setShader(this->m_unlitShader);

			int sectors = detail, stacks = detail;
			std::vector<Vertex> verts;
			for (int i = 0; i <= stacks; ++i)
			{
				float stackAngle = glm::pi<float>() / 2.0f - (float)i * glm::pi<float>() / (float)stacks;
				float xy = body.radius * cosf(stackAngle);
				float z = body.radius * sinf(stackAngle);
				for (int j = 0; j <= sectors; ++j)
				{
					float sectorAngle = 2.0f * glm::pi<float>() * (float)j / (float)sectors;
					Vertex v;
					v.position.x = xy * cosf(sectorAngle);
					v.position.y = z;
					v.position.z = xy * sinf(sectorAngle);
					v.normal = glm::normalize(v.position);
					v.texCoord = {(float)j / (float)sectors, (float)i / (float)stacks};
					v.color = body.color;
					verts.push_back(v);
				}
			}
			this->m_baseVertices[bi] = std::move(verts);

			MaterialProperties props;
			if (body.name == "Sun")
			{
				props.emissive = body.color;
				props.diffuse = body.color;
			}
			else
			{
				props.diffuse = body.color;
				props.emissive = body.color; // raycaster controls actual brightness via vertex colors
			}
			body.material.setProperties(props);
			body.angle = ((float)(std::hash<std::string>{}(body.name) % 1000)) / 1000.0f * 6.28f;

			// Saturn's rings
			if (body.name == "Saturn")
			{
				body.hasRing = true;
				body.ringTilt = 26.7f;
				body.ringInnerRadius = body.radius * 1.3f;
				body.ringOuterRadius = body.radius * 2.4f;
				Color ringInner = {0.85f, 0.75f, 0.55f, 0.8f};
				Color ringOuter = {0.65f, 0.55f, 0.40f, 0.3f};
				body.ringMesh = Mesh::generateRing(body.ringInnerRadius, body.ringOuterRadius, 64, ringInner, ringOuter,
												   Graphics::BufferUsage::Dynamic);

				// Store base vertex colors for shadow computation
				int segments = 64;
				body.ringBaseVertices.clear();
				for (int ri = 0; ri <= segments; ++ri)
				{
					float angle = 2.0f * glm::pi<float>() * (float)ri / (float)segments;
					float c = cosf(angle), s = sinf(angle);
					Vertex vi, vo;
					vi.position = {body.ringInnerRadius * c, 0.0f, body.ringInnerRadius * s};
					vi.normal = {0, 1, 0};
					vi.color = ringInner;
					vo.position = {body.ringOuterRadius * c, 0.0f, body.ringOuterRadius * s};
					vo.normal = {0, 1, 0};
					vo.color = ringOuter;
					body.ringBaseVertices.push_back(vi);
					body.ringBaseVertices.push_back(vo);
				}

				body.ringMaterial.setShader(this->m_unlitShader);
				MaterialProperties ringProps;
				ringProps.emissive = {0.4f, 0.35f, 0.25f, 1.0f};
				ringProps.diffuse = {0.6f, 0.5f, 0.35f, 0.6f};
				body.ringMaterial.setProperties(ringProps);
			}
		}
	}

	void initTrails()
	{
		this->m_trails.resize(this->m_bodies.size());
		for (auto &trail : this->m_trails)
			trail.init();
	}

	void initOrbits()
	{
		for (const auto &body : this->m_bodies)
		{
			if (body.orbitRadius > 0.0f)
				this->m_orbitCircles.push_back(Mesh::generateCircle(body.orbitRadius, 128,
																	{body.color.r * 0.5f, body.color.g * 0.5f, body.color.b * 0.5f, 0.3f}));
		}
	}

	std::shared_ptr<Graphics::IShader> m_defaultShader;
	std::shared_ptr<Graphics::IShader> m_lineShader;
	std::shared_ptr<Graphics::IShader> m_unlitShader;

	std::vector<CelestialBody> m_bodies;
	std::vector<std::vector<Vertex>> m_baseVertices;
	std::vector<TrailRenderer> m_trails;
	std::vector<Mesh> m_orbitCircles;

	SpacetimeGrid m_spacetime;
	AsteroidBelt m_asteroidBelt;
	Raycaster m_raycaster;
	SolarSystemPanel m_panel;

	float m_timeScale = 1.0f;
	bool m_paused = false;
	bool m_showOrbits = true;
	bool m_showTrails = true;
	bool m_showSpacetime = true;
	bool m_showUI = true;
	bool m_showEclipses = true;
	int m_followIdx = -1;

	Vec2 m_clickStart{0.0f};
};

GE_APP(SolarSystemApp)
