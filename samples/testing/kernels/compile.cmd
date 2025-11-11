slangc test.slang -profile sm_6_0 -target spirv -o test.spv -entry entry -bindless-space-index 1
slangc image.slang -profile sm_6_0 -target spirv -o image.spv -entry entry -bindless-space-index 1
slangc buffer-fill.slang -profile sm_6_0 -target spirv -o buffer-fill.spv -entry entry -bindless-space-index 1

slangc graphics-test.slang -profile vs_6_0 -target spirv -o graphics-test-vert.spv -entry vertex_entry -bindless-space-index 1
slangc graphics-test.slang -profile ps_6_0 -target spirv -o graphics-test-frag.spv -entry fragment_entry -bindless-space-index 1
pause
