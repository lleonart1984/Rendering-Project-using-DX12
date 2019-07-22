#include "..\CommonGI\Parameters.h"

int3 FromPositionToCell(float3 P) {
	return (int3)((P + 1)*0.5 / PHOTON_CELL_SIZE);
}

int GetHashIndex(int3 cell);

int GetHashIndex(float3 P) {
	int3 cell = FromPositionToCell(P);
	return GetHashIndex(cell);
}
