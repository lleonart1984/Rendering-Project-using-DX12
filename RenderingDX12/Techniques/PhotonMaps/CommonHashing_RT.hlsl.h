
int3 FromPositionToCell(float3 P) {
	return (int3)((P + 1)*0.5 / PHOTON_CELL_SIZE);
}

#if USE_VOLUME_GRID
int GetHashIndex(int3 cell)
{
	cell = clamp(cell, 0, PHOTON_GRID_SIZE - 1);
	return cell.x + (cell.y + cell.z * PHOTON_GRID_SIZE) * PHOTON_GRID_SIZE;
}
#else
static int PRIME1 = (2 * 3 * 5 * 7 * 11 * 13 * 17 + 1);
static int PRIME2 = (2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23 * 29 + 1);
static int PRIME3 = (2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23 * 29 * 31 * 37 + 1);

int GetHashIndex(int3 cell) {
	int h = (cell.x * PRIME1) ^ (cell.y * PRIME2) ^ (cell.z * PRIME3);
	return ((h % HASH_CAPACITY) + HASH_CAPACITY) % HASH_CAPACITY;
}
#endif