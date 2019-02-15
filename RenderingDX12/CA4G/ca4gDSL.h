#pragma once

// Technique's methods

#define render_target getManager()->getBackBuffer()

#define perform(method) getManager()->Perform(process(method))

#define perform_async(method) getManager()->PerformAsync(processPtr(method))

#define flush_to_gpu(engine_mask) getManager()->Flush(engine_mask)

#define flush_all_to_gpu getManager()->Flush(ENGINE_MASK_ALL)

#define signal(engine_mask) getManager()->SendSignal(engine_mask)

#define wait_for(signal) getManager()->WaitFor(signal)

#define set_tag(t) getManager()->Tag = t

#define _ this

#define gBind(x) (x) =

#define gCreate ->creating->

#define gLoad ->loading->

#define gSet ->setting->

#define gDraw ->drawer->

#define gOpen(pipeline) (pipeline)->Open();

#define gClose(pipeline) (pipeline)->Close();

#define gClear ->clearing->

#define gCopy ->copying->
