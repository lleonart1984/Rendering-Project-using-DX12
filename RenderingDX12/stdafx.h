#pragma once

#include "Techniques/Sample/Sample.h"
#include "Techniques/RetainedBasic/RetainedBasicRenderer.h"
#include "Techniques/Tutorials/Tutorial1.h"
#include "Techniques/Tutorials/Tutorial2.h"
#include "Techniques/Tutorials/Tutorial3.h"
#include "Techniques/Tutorials/Tutorial4.h"
#include "Techniques/Tutorials/Tutorial5.h"
#include "Techniques/Tutorials/Tutorial6.h"
#include "Techniques/Tutorials/Tutorial7.h"

#define BASIC_OBJ 0
#define SPONZA_OBJ 1
#define SIBENIK_OBJ 2
#define SANMIGUEL_OBJ 3

#define USE_SCENE SPONZA_OBJ 

//class NoTechnique : public Technique {}; gObj<NoTechnique> technique;

//gObj<SampleTechnique> technique;
gObj<RetainedBasicRenderer> technique;
//gObj<Tutorial1> technique;
//gObj<Tutorial2> technique;
//gObj<Tutorial3> technique;
//gObj<Tutorial4> technique;
//gObj<Tutorial5> technique;
//gObj<Tutorial6> technique;
//gObj<Tutorial7> technique;
