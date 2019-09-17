#pragma once

struct RaymarchingDebug {
    int CountSteps;
    int CountHits;
    float2 rem;
};

class DebugRaymarchRT {
public:
    bool CountHits;
    bool CountSteps;
protected:
    virtual void Boo() {}
};