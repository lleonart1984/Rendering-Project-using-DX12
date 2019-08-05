struct Photon {
	float3 Intensity;
#ifdef PHOTON_WITH_POSITION
	float3 Position;
#endif
#ifdef PHOTON_WITH_RADIUS
	float Radius;
#endif
#ifdef PHOTON_WITH_NORMAL
	float3 Normal;
#endif
#ifdef PHOTON_WITH_DIRECTION
	float3 Direction;
#endif
};
