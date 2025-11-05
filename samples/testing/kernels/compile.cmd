slangc test.slang -profile sm_6_0 -target spirv -o test.spv -entry entry
slangc buffer-fill.slang -profile sm_6_0 -target spirv -o buffer-fill.spv -entry entry

slangc graphics-test.slang -profile vs_6_0 -target spirv -o graphics-test-vert.spv -entry vertex_entry
slangc graphics-test.slang -profile ps_6_0 -target spirv -o graphics-test-frag.spv -entry fragment_entry
pause
