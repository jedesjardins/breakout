mkdir ../build/shaders
glslc -fshader-stage=vert ../src/shaders/pos.vert -o ../build/shaders/pos_vert.spv
glslc -fshader-stage=frag ../src/shaders/pos.frag -o ../build/shaders/pos_frag.spv