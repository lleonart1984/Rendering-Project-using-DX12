static int PRIME1 = (2 * 3 * 5 * 7 * 11 * 13 * 17 + 1);
static int PRIME2 = (2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23 * 29 + 1);
static int PRIME3 = (2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23 * 29 * 31 * 37 + 1);

int GetHashIndex(int3 cell) {
	int h = (cell.x * PRIME1) ^ (cell.y * PRIME2) ^ (cell.z * PRIME3);

	return ((h % HASH_CAPACITY) + HASH_CAPACITY) % HASH_CAPACITY;
}