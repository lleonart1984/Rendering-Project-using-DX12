float1 reluActivation(float1 x) { return max(0, x); }

float2 reluActivation(float2 x) { return max(0, x); }

float3 reluActivation(float3 x) { return max(0, x); }

float4 reluActivation(float4 x) { return max(0, x); }

float1 softplusActivation(float1 x) { return log(1 + exp(x)); }

float2 softplusActivation(float2 x) { return log(1 + exp(x)); }

float3 softplusActivation(float3 x) { return log(1 + exp(x)); }

float4 softplusActivation(float4 x) { return log(1 + exp(x)); }

float1 tanhActivation(float1 x) { return tanh(x); }

float2 tanhActivation(float2 x) { return tanh(x); }

float3 tanhActivation(float3 x) { return tanh(x); }

float4 tanhActivation(float4 x) { return tanh(x); }

cbuffer Models : register (b7) { float4x4 ModelSlices0; float4 ModelSlices1; float4x4 ModelSlices2; float4 ModelSlices3; float4x1 ModelSlices4; float1 ModelSlices5; float4x4 ModelSlices6; float4x4 ModelSlices7; float4x4 ModelSlices8; float4x4 ModelSlices9; float4x4 ModelSlices10; float4x4 ModelSlices11; float4x4 ModelSlices12; float4x4 ModelSlices13; float4x4 ModelSlices14; float4 ModelSlices15; float4 ModelSlices16; float4 ModelSlices17; float4x4 ModelSlices18; float4x4 ModelSlices19; float4x4 ModelSlices20; float4x4 ModelSlices21; float4x4 ModelSlices22; float4x4 ModelSlices23; float4x4 ModelSlices24; float4x4 ModelSlices25; float4x4 ModelSlices26; float4 ModelSlices27; float4 ModelSlices28; float4 ModelSlices29; float4x4 ModelSlices30; float4x4 ModelSlices31; float4x4 ModelSlices32; float4x4 ModelSlices33; float4x4 ModelSlices34; float4x4 ModelSlices35; float4x4 ModelSlices36; float4x4 ModelSlices37; float4x4 ModelSlices38; float4 ModelSlices39; float4 ModelSlices40; float4 ModelSlices41; float4x3 ModelSlices42; float4x3 ModelSlices43; float4x3 ModelSlices44; float3 ModelSlices45; float4x4 ModelSlices46; float4x4 ModelSlices47; float4x4 ModelSlices48; float4x4 ModelSlices49; float4x4 ModelSlices50; float4x4 ModelSlices51; float4x4 ModelSlices52; float4x4 ModelSlices53; float4x4 ModelSlices54; float4x4 ModelSlices55; float4x4 ModelSlices56; float4x4 ModelSlices57; float4x4 ModelSlices58; float4x4 ModelSlices59; float4x4 ModelSlices60; float4x4 ModelSlices61; float4 ModelSlices62; float4 ModelSlices63; float4 ModelSlices64; float4 ModelSlices65; float4x4 ModelSlices66; float4x4 ModelSlices67; float4x4 ModelSlices68; float4x4 ModelSlices69; float4x4 ModelSlices70; float4x4 ModelSlices71; float4x4 ModelSlices72; float4x4 ModelSlices73; float4x4 ModelSlices74; float4x4 ModelSlices75; float4x4 ModelSlices76; float4x4 ModelSlices77; float4x4 ModelSlices78; float4x4 ModelSlices79; float4x4 ModelSlices80; float4x4 ModelSlices81; float4 ModelSlices82; float4 ModelSlices83; float4 ModelSlices84; float4 ModelSlices85; float4x4 ModelSlices86; float4x4 ModelSlices87; float4x4 ModelSlices88; float4x4 ModelSlices89; float4x4 ModelSlices90; float4x4 ModelSlices91; float4x4 ModelSlices92; float4x4 ModelSlices93; float4x4 ModelSlices94; float4x4 ModelSlices95; float4x4 ModelSlices96; float4x4 ModelSlices97; float4x4 ModelSlices98; float4x4 ModelSlices99; float4x4 ModelSlices100; float4x4 ModelSlices101; float4 ModelSlices102; float4 ModelSlices103; float4 ModelSlices104; float4 ModelSlices105; float4x4 ModelSlices106; float4x2 ModelSlices107; float4x4 ModelSlices108; float4x2 ModelSlices109; float4x4 ModelSlices110; float4x2 ModelSlices111; float4x4 ModelSlices112; float4x2 ModelSlices113; float4 ModelSlices114; float2 ModelSlices115; }
// Values 1532
//-1.4647857,-0.42826492,0.0013390315,-0.027267234,1.9631772,0.23757873,-0.0020370884,0.3737895,-1.8965818,-0.26607674,0.0007524612,-0.34710613,-1.5985893,-0.21281911,-0.00064108754,-0.2896381,0.7190652,-1.7997388,1.5423869,1.3695614,-0.5154928,-8.022841,0.2165962,1.2858816,0.27903977,-7.2458677,0.8634012,-0.18221265,0.42385444,-10.611959,1.2483487,0.99954826,0.18819262,-6.2749906,0.3309136,-7.3158817,0.38619936,0.3705537,0.63924336,0.13597777,0.28611255,0.8962767,0.9290795,-2.5632136,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-8.774182e-05,0.0,0.0,0.0,0.9255522,0.41462663,0.2256235,0.00414795,-0.7786284,-1.1499567,-0.30250007,0.0020423084,1.0476426,0.3274382,0.13693495,-0.004216575,0.19701533,-0.7395321,0.4402949,-0.00011078982,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,1.217227,-0.7776381,0.9538226,0.20390716,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.1635268,0.47841036,0.3801087,-0.28394175,-0.18315592,-0.6865864,-0.27191117,-0.20040253,-0.67976075,-0.48755664,-0.54452306,-0.63608444,0.13916163,0.06282353,-0.8440717,-0.2327592,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.3527436,-0.15526,-0.023627017,0.83549196,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-0.09213692,0.058302205,-0.0231775,0.42448258,-0.31112236,-0.3370085,0.27191323,-0.723563,0.14158297,0.20348573,0.09355828,0.39322242,-0.70731956,-0.46822667,-0.2398889,-0.2416297,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-0.22911927,-0.120498806,-0.026487859,-0.21361564,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.46676952,0.23806046,-0.16086742,0.08300621,0.21523386,-0.12176295,-0.30000535,0.9243173,0.08341274,0.58993834,-0.029834576,0.13168065,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-0.17423737,-0.1276763,-0.117987655,0.0,-0.22125627,0.03388198,-0.1346194,0.04082939,-0.8635041,-0.4739341,-0.1708617,-0.4937302,-0.314263,-0.61796826,-0.080566004,-0.08501824,0.18831664,0.33234015,-0.05323136,0.08699551,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.17818955,-0.32257217,0.65886194,0.6248311,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.021694936,-0.0073400754,0.003928341,0.02739275,0.19251576,0.051194604,-0.053545944,-0.09787993,0.03763834,0.5699301,0.008806155,-1.2103598,0.77712303,0.32723936,0.06403976,-0.02887953,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,1.4667647,-0.21735749,0.4467751,-1.4974895,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.47017896,-0.020598216,-0.022962948,0.31709784,0.338083,-0.44989046,-0.074715964,0.66618264,-0.4524785,-0.23476051,-0.044662375,-0.18959358,0.16153713,0.32541636,0.16546361,1.2241935,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-0.5745212,0.0048284153,-0.5753595,0.23922971,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.17851326,0.13587114,-0.03003536,0.24555103,-0.021877235,-0.26714474,0.02069091,0.14613815,0.011808174,0.49991292,0.0012738878,-0.22097436,0.19420603,-0.055961106,-0.23957144,0.0996349,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-0.14149353,-0.10642349,-0.4737218,0.18177813,0.0,0.0,0.0,0.0
void LengthGen_layer_0(in float4 i0, out float4 o0) {
	o0 = reluActivation(mul(i0, (float4x4)ModelSlices0) + (float4)ModelSlices1);
}

void LengthGen_layer_1(in float4 i0, out float4 o0) {
	o0 = reluActivation(mul(i0, (float4x4)ModelSlices2) + (float4)ModelSlices3);
}

void LengthGen_layer_2(in float4 i0, out float1 o0) {
	o0 = softplusActivation(mul(i0, (float4x1)ModelSlices4) + (float1)ModelSlices5);
}

void LengthGen(float _input[4], out float _output[1]) {
	float4 n_0_0 = (float4)0;
	float4 n_1_0 = (float4)0;
	float4 n_2_0 = (float4)0;
	float1 n_3_0 = (float1)0;
	n_0_0[0] = _input[0];
	n_0_0[1] = _input[1];
	n_0_0[2] = _input[2];
	n_0_0[3] = _input[3];
	LengthGen_layer_0(n_0_0, n_1_0);
	LengthGen_layer_1(n_1_0, n_2_0);
	LengthGen_layer_2(n_2_0, n_3_0);
	_output[0] = n_3_0[0];
}
void PathGen_layer_0(in float4 i0, in float4 i1, in float4 i2, out float4 o0, out float4 o1, out float4 o2) {
	o0 = reluActivation(mul(i0, (float4x4)ModelSlices6) + mul(i1, (float4x4)ModelSlices9) + mul(i2, (float4x4)ModelSlices12) + (float4)ModelSlices15);
	o1 = reluActivation(mul(i0, (float4x4)ModelSlices7) + mul(i1, (float4x4)ModelSlices10) + mul(i2, (float4x4)ModelSlices13) + (float4)ModelSlices16);
	o2 = reluActivation(mul(i0, (float4x4)ModelSlices8) + mul(i1, (float4x4)ModelSlices11) + mul(i2, (float4x4)ModelSlices14) + (float4)ModelSlices17);
}

void PathGen_layer_1(in float4 i0, in float4 i1, in float4 i2, out float4 o0, out float4 o1, out float4 o2) {
	o0 = reluActivation(mul(i0, (float4x4)ModelSlices18) + mul(i1, (float4x4)ModelSlices21) + mul(i2, (float4x4)ModelSlices24) + (float4)ModelSlices27);
	o1 = reluActivation(mul(i0, (float4x4)ModelSlices19) + mul(i1, (float4x4)ModelSlices22) + mul(i2, (float4x4)ModelSlices25) + (float4)ModelSlices28);
	o2 = reluActivation(mul(i0, (float4x4)ModelSlices20) + mul(i1, (float4x4)ModelSlices23) + mul(i2, (float4x4)ModelSlices26) + (float4)ModelSlices29);
}

void PathGen_layer_2(in float4 i0, in float4 i1, in float4 i2, out float4 o0, out float4 o1, out float4 o2) {
	o0 = reluActivation(mul(i0, (float4x4)ModelSlices30) + mul(i1, (float4x4)ModelSlices33) + mul(i2, (float4x4)ModelSlices36) + (float4)ModelSlices39);
	o1 = reluActivation(mul(i0, (float4x4)ModelSlices31) + mul(i1, (float4x4)ModelSlices34) + mul(i2, (float4x4)ModelSlices37) + (float4)ModelSlices40);
	o2 = reluActivation(mul(i0, (float4x4)ModelSlices32) + mul(i1, (float4x4)ModelSlices35) + mul(i2, (float4x4)ModelSlices38) + (float4)ModelSlices41);
}

void PathGen_layer_3(in float4 i0, in float4 i1, in float4 i2, out float3 o0) {
	o0 = tanhActivation(mul(i0, (float4x3)ModelSlices42) + mul(i1, (float4x3)ModelSlices43) + mul(i2, (float4x3)ModelSlices44) + (float3)ModelSlices45);
}

void PathGen(float _input[12], out float _output[3]) {
	float4 n_0_0 = (float4)0;
	float4 n_0_1 = (float4)0;
	float4 n_0_2 = (float4)0;
	float4 n_1_0 = (float4)0;
	float4 n_1_1 = (float4)0;
	float4 n_1_2 = (float4)0;
	float4 n_2_0 = (float4)0;
	float4 n_2_1 = (float4)0;
	float4 n_2_2 = (float4)0;
	float4 n_3_0 = (float4)0;
	float4 n_3_1 = (float4)0;
	float4 n_3_2 = (float4)0;
	float3 n_4_0 = (float3)0;
	n_0_0[0] = _input[0];
	n_0_0[1] = _input[1];
	n_0_0[2] = _input[2];
	n_0_0[3] = _input[3];
	n_0_1[0] = _input[4];
	n_0_1[1] = _input[5];
	n_0_1[2] = _input[6];
	n_0_1[3] = _input[7];
	n_0_2[0] = _input[8];
	n_0_2[1] = _input[9];
	n_0_2[2] = _input[10];
	n_0_2[3] = _input[11];
	PathGen_layer_0(n_0_0, n_0_1, n_0_2, n_1_0, n_1_1, n_1_2);
	PathGen_layer_1(n_1_0, n_1_1, n_1_2, n_2_0, n_2_1, n_2_2);
	PathGen_layer_2(n_2_0, n_2_1, n_2_2, n_3_0, n_3_1, n_3_2);
	PathGen_layer_3(n_3_0, n_3_1, n_3_2, n_4_0);
	_output[0] = n_4_0[0];
	_output[1] = n_4_0[1];
	_output[2] = n_4_0[2];
}
void ScatGen_layer_0(in float4 i0, in float4 i1, in float4 i2, in float4 i3, out float4 o0, out float4 o1, out float4 o2, out float4 o3) {
	o0 = reluActivation(mul(i0, (float4x4)ModelSlices46) + mul(i1, (float4x4)ModelSlices50) + mul(i2, (float4x4)ModelSlices54) + mul(i3, (float4x4)ModelSlices58) + (float4)ModelSlices62);
	o1 = reluActivation(mul(i0, (float4x4)ModelSlices47) + mul(i1, (float4x4)ModelSlices51) + mul(i2, (float4x4)ModelSlices55) + mul(i3, (float4x4)ModelSlices59) + (float4)ModelSlices63);
	o2 = reluActivation(mul(i0, (float4x4)ModelSlices48) + mul(i1, (float4x4)ModelSlices52) + mul(i2, (float4x4)ModelSlices56) + mul(i3, (float4x4)ModelSlices60) + (float4)ModelSlices64);
	o3 = reluActivation(mul(i0, (float4x4)ModelSlices49) + mul(i1, (float4x4)ModelSlices53) + mul(i2, (float4x4)ModelSlices57) + mul(i3, (float4x4)ModelSlices61) + (float4)ModelSlices65);
}

void ScatGen_layer_1(in float4 i0, in float4 i1, in float4 i2, in float4 i3, out float4 o0, out float4 o1, out float4 o2, out float4 o3) {
	o0 = reluActivation(mul(i0, (float4x4)ModelSlices66) + mul(i1, (float4x4)ModelSlices70) + mul(i2, (float4x4)ModelSlices74) + mul(i3, (float4x4)ModelSlices78) + (float4)ModelSlices82);
	o1 = reluActivation(mul(i0, (float4x4)ModelSlices67) + mul(i1, (float4x4)ModelSlices71) + mul(i2, (float4x4)ModelSlices75) + mul(i3, (float4x4)ModelSlices79) + (float4)ModelSlices83);
	o2 = reluActivation(mul(i0, (float4x4)ModelSlices68) + mul(i1, (float4x4)ModelSlices72) + mul(i2, (float4x4)ModelSlices76) + mul(i3, (float4x4)ModelSlices80) + (float4)ModelSlices84);
	o3 = reluActivation(mul(i0, (float4x4)ModelSlices69) + mul(i1, (float4x4)ModelSlices73) + mul(i2, (float4x4)ModelSlices77) + mul(i3, (float4x4)ModelSlices81) + (float4)ModelSlices85);
}

void ScatGen_layer_2(in float4 i0, in float4 i1, in float4 i2, in float4 i3, out float4 o0, out float4 o1, out float4 o2, out float4 o3) {
	o0 = reluActivation(mul(i0, (float4x4)ModelSlices86) + mul(i1, (float4x4)ModelSlices90) + mul(i2, (float4x4)ModelSlices94) + mul(i3, (float4x4)ModelSlices98) + (float4)ModelSlices102);
	o1 = reluActivation(mul(i0, (float4x4)ModelSlices87) + mul(i1, (float4x4)ModelSlices91) + mul(i2, (float4x4)ModelSlices95) + mul(i3, (float4x4)ModelSlices99) + (float4)ModelSlices103);
	o2 = reluActivation(mul(i0, (float4x4)ModelSlices88) + mul(i1, (float4x4)ModelSlices92) + mul(i2, (float4x4)ModelSlices96) + mul(i3, (float4x4)ModelSlices100) + (float4)ModelSlices104);
	o3 = reluActivation(mul(i0, (float4x4)ModelSlices89) + mul(i1, (float4x4)ModelSlices93) + mul(i2, (float4x4)ModelSlices97) + mul(i3, (float4x4)ModelSlices101) + (float4)ModelSlices105);
}

void ScatGen_layer_3(in float4 i0, in float4 i1, in float4 i2, in float4 i3, out float4 o0, out float2 o1) {
	o0 = tanhActivation(mul(i0, (float4x4)ModelSlices106) + mul(i1, (float4x4)ModelSlices108) + mul(i2, (float4x4)ModelSlices110) + mul(i3, (float4x4)ModelSlices112) + (float4)ModelSlices114);
	o1 = tanhActivation(mul(i0, (float4x2)ModelSlices107) + mul(i1, (float4x2)ModelSlices109) + mul(i2, (float4x2)ModelSlices111) + mul(i3, (float4x2)ModelSlices113) + (float2)ModelSlices115);
}

void ScatGen(float _input[16], out float _output[6]) {
	float4 n_0_0 = (float4)0;
	float4 n_0_1 = (float4)0;
	float4 n_0_2 = (float4)0;
	float4 n_0_3 = (float4)0;
	float4 n_1_0 = (float4)0;
	float4 n_1_1 = (float4)0;
	float4 n_1_2 = (float4)0;
	float4 n_1_3 = (float4)0;
	float4 n_2_0 = (float4)0;
	float4 n_2_1 = (float4)0;
	float4 n_2_2 = (float4)0;
	float4 n_2_3 = (float4)0;
	float4 n_3_0 = (float4)0;
	float4 n_3_1 = (float4)0;
	float4 n_3_2 = (float4)0;
	float4 n_3_3 = (float4)0;
	float4 n_4_0 = (float4)0;
	float2 n_4_1 = (float2)0;
	n_0_0[0] = _input[0];
	n_0_0[1] = _input[1];
	n_0_0[2] = _input[2];
	n_0_0[3] = _input[3];
	n_0_1[0] = _input[4];
	n_0_1[1] = _input[5];
	n_0_1[2] = _input[6];
	n_0_1[3] = _input[7];
	n_0_2[0] = _input[8];
	n_0_2[1] = _input[9];
	n_0_2[2] = _input[10];
	n_0_2[3] = _input[11];
	n_0_3[0] = _input[12];
	n_0_3[1] = _input[13];
	n_0_3[2] = _input[14];
	n_0_3[3] = _input[15];
	ScatGen_layer_0(n_0_0, n_0_1, n_0_2, n_0_3, n_1_0, n_1_1, n_1_2, n_1_3);
	ScatGen_layer_1(n_1_0, n_1_1, n_1_2, n_1_3, n_2_0, n_2_1, n_2_2, n_2_3);
	ScatGen_layer_2(n_2_0, n_2_1, n_2_2, n_2_3, n_3_0, n_3_1, n_3_2, n_3_3);
	ScatGen_layer_3(n_3_0, n_3_1, n_3_2, n_3_3, n_4_0, n_4_1);
	_output[0] = n_4_0[0];
	_output[1] = n_4_0[1];
	_output[2] = n_4_0[2];
	_output[3] = n_4_0[3];
	_output[4] = n_4_1[0];
	_output[5] = n_4_1[1];
}
