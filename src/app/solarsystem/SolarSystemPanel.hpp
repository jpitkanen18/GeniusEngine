#pragma once
// Solar System control panel — demonstrates the Panel abstraction.
// All UI layout lives here; the app just passes data references.

#include "../../ui/Panel.hpp"
#include "CelestialBody.hpp"
#include "Raycaster.hpp"
#include "AsteroidBelt.hpp"
#include "SpacetimeGrid.hpp"
#include "../../engine/Camera.hpp"
#include <omp.h>

namespace GE
{

	class SolarSystemPanel : public UI::Panel
	{
	public:
		SolarSystemPanel() : Panel("Solar System Controls") {}

		// --- Data bindings (set by the app each frame) ---
		std::vector<CelestialBody> *bodies = nullptr;
		Raycaster *raycaster = nullptr;
		AsteroidBelt *asteroidBelt = nullptr;
		SpacetimeGrid *spacetime = nullptr;
		Camera *cam = nullptr;

		int *followIdx = nullptr;
		float *timeScale = nullptr;
		bool *paused = nullptr;
		bool *showOrbits = nullptr;
		bool *showTrails = nullptr;
		bool *showSpacetime = nullptr;
		bool *showEclipses = nullptr;
		float *dt = nullptr;

		// Device info (set once from context)
		std::string backendName;
		std::string deviceName;
		std::string apiVersion;
		std::string driverVersion;

	protected:
		void layout() override
		{
			if (!this->bodies || !this->followIdx)
				return;

			auto &b = *this->bodies;
			int &follow = *this->followIdx;

			// Follow indicator
			if (follow >= 0 && follow < (int)b.size())
				labelColored({0.3f, 1.0f, 0.5f, 1.0f}, "Following: " + b[follow].name);
			else
				label("Free camera (WASD/QE/RF)");

			// Body selector
			std::string preview = (follow >= 0) ? b[follow].name : "None";
			if (beginCombo("Follow Body", preview))
			{
				if (selectableItem("None (free cam)", follow < 0))
					follow = -1;
				for (size_t i = 0; i < b.size(); ++i)
				{
					if (selectableItem(b[i].name, follow == (int)i))
						follow = (int)i;
				}
				endCombo();
			}
			separator();

			// Simulation
			if (this->timeScale)
				slider("Time Scale", *this->timeScale, 0.0f, 10.0f);
			if (this->paused)
				checkbox("Paused", *this->paused);
			separator();

			// Visualization toggles
			label("Visualization");
			if (this->showOrbits)
				checkbox("Orbits", *this->showOrbits);
			sameLine();
			if (this->showTrails)
				checkbox("Trails", *this->showTrails);
			if (this->showSpacetime)
				checkbox("Spacetime", *this->showSpacetime);
			sameLine();
			if (this->asteroidBelt)
				checkbox("Asteroids", this->asteroidBelt->visible);
			if (this->showEclipses)
				checkbox("Eclipses", *this->showEclipses);
			sameLine();
			if (this->raycaster)
				checkbox("Light Rays", this->raycaster->showRays);

			if (this->showSpacetime && *this->showSpacetime && this->spacetime)
				slider("Curvature", this->spacetime->curvatureStrength, 0.1f, 10.0f);
			if (this->showEclipses && *this->showEclipses && this->raycaster)
				slider("Lensing", this->raycaster->lensingScale, 0.0f, 50.0f);
			separator();

			// Body list
			label("Bodies (0-9 keys to follow)");
			for (size_t i = 0; i < b.size(); ++i)
			{
				std::string name = b[i].name;
				if (follow == (int)i)
					name += " *";

				if (treeNode(name))
				{
					if (button("Follow##" + std::to_string(i)))
						follow = (int)i;
					sameLine();
					if (button("Center##" + std::to_string(i)) && this->cam)
					{
						follow = (int)i;
						this->cam->lookAt(
							b[i].position + Vec3(0, b[i].radius * 5, b[i].radius * 5),
							b[i].position, Vec3(0, 1, 0));
					}

					Vec3 pos = b[i].position;
					label("Pos: (" + std::to_string(pos.x).substr(0, 6) + ", " +
						  std::to_string(pos.y).substr(0, 6) + ", " +
						  std::to_string(pos.z).substr(0, 6) + ")");

					if (i > 0 && this->showEclipses && *this->showEclipses &&
						this->raycaster && this->raycaster->bodyIllum[i].shadowFactor > 0.01f)
					{
						int occ = this->raycaster->bodyIllum[i].primaryOccluder;
						std::string occName = (occ > 0) ? b[occ].name : "?";
						labelColored({1.0f, 0.3f, 0.2f, 1.0f},
									 "ECLIPSE by " + occName + " (" +
										 std::to_string((int)(this->raycaster->bodyIllum[i].shadowFactor * 100)) + "%)");
					}
					treePop();
				}
			}
			separator();

			// Status
			float fps = (this->dt && *this->dt > 0.0001f) ? 1.0f / *this->dt : 0.0f;
			label("FPS: " + std::to_string((int)fps) +
				  " | Rays/body: " + std::to_string(Raycaster::TARGETED_RAYS) +
				  " | Threads: " + std::to_string(omp_get_max_threads()));
			label("LMB=Click body/Look  RMB=Orbit  WASD=Move  Scroll=Zoom");

			separator();
			label("Graphics");
			if (!this->backendName.empty())
				label("  API: " + this->backendName);
			if (!this->deviceName.empty())
				label("  Device: " + this->deviceName);
			if (!this->apiVersion.empty())
				label("  Version: " + this->apiVersion);
			if (!this->driverVersion.empty())
				label("  Driver: " + this->driverVersion);
		}
	};

} // namespace GE
