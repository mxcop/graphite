slangc test.slang -profile sm_6_0 -target spirv -o test.spv -entry entry
slangc image.slang -profile sm_6_0 -target spirv -o image.spv -entry entry
slangc buffer-fill.slang -profile sm_6_0 -target spirv -o buffer-fill.spv -entry entry
pause
