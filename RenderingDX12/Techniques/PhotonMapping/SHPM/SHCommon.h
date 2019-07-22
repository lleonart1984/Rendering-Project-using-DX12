int3 FromPositionToCell(float3 P) {
	return (int3)floor(P*PHOTON_GRID_SIZE);
}

static int PRIME1 = (2 * 3 * 5 * 7 * 11 * 13 * 17 + 1);
static int PRIME2 = (2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23 * 29 + 1);
static int PRIME3 = (2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23 * 29 * 31 * 37 + 1);

int GetHashCode(int3 cell) {
	return
		(cell.x * PRIME1) ^ (cell.y * PRIME2) ^ (cell.z * PRIME3);
		/*(cell.x * 12341121113) ^ (cell.y * 23121234557) ^ (cell.z * 72929112923)*/;
}

int FromCellToCellIndex(int3 cell)
{
	return ((GetHashCode(cell) % HASH_CAPACITY) + HASH_CAPACITY) % HASH_CAPACITY;
}

int FromPositionToCellIndex(float3 P) {
	return FromCellToCellIndex(FromPositionToCell(P));
}
