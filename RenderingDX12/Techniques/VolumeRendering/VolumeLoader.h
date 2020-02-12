#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../CommonGI/Parameters.h"

class VolumeLoader : public Technique, public IHasVolume {
public:
	int VolumeWidth, VolumeHeight, VolumeSlices;
	gObj<Texture3D> VolumeData;
	gObj<Texture3D> SumData;

protected:
	void Startup() {
		perform(CreatingAssets);
	}

	void CreatingAssets(gObj<GraphicsManager> manager) {
		// loading scene textures
		VolumeWidth = this->Volume->width;
		VolumeHeight = this->Volume->height;
		VolumeSlices = this->Volume->slices;

		// load full vertex buffer of all scene geometries
		VolumeData = _ gCreate ReadonlyTexture3D<float>(VolumeWidth, VolumeHeight, VolumeSlices);
		VolumeData->SetDebugName(L"Volume Data Buffer");
		manager gCopy PtrData(VolumeData, Volume->data);

		SumData = _ gCreate ReadonlyTexture3D<float2>(VolumeWidth, VolumeHeight, VolumeSlices);
		SumData->SetDebugName(L"Volume Data Buffer for sums");
		manager gCopy PtrData(SumData, Volume->sums);
	}
};

struct VolumeInfo {
	int Width;
	int Height;
	int Slices;
	float Density;
	float Absortion;
};