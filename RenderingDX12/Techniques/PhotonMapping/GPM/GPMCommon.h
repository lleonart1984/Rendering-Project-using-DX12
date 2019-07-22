int3 FromPositionToCell(float3 P) {
	return (int3)((P + 1)*0.5*PHOTON_GRID_SIZE);
}

int FromCellToCellIndex(int3 cell)
{
	cell = clamp(cell, 0, PHOTON_GRID_SIZE - 1);
	return cell.x + cell.y * PHOTON_GRID_SIZE + cell.z * PHOTON_GRID_SIZE * PHOTON_GRID_SIZE;
}

int FromPositionToCellIndex(float3 P) {
	return FromCellToCellIndex(FromPositionToCell(P));
}