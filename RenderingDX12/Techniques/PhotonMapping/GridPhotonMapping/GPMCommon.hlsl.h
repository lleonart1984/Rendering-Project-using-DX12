/// Common code for fixed grid based photon-mapping techniques

int GetHashIndex(int3 cell)
{
	cell = clamp(cell, 0, PHOTON_GRID_SIZE - 1);
	return cell.x + (cell.y + cell.z * PHOTON_GRID_SIZE) * PHOTON_GRID_SIZE;
}
