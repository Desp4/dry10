@echo OFF
rem %1 - processing dir %2 - glslc path %3 spir-v dir %4 pcsr path %5 out header dir

for %%f in (%1/*.vert) do (
    echo compiling shader %%~nf
    echo compiling %%~nf.vert
    %2 %1/%%~nf.vert -o %3/%%~nf.vert.spv
    echo compiling %%~nf.frag
    %2 %1/%%~nf.frag -o %3/%%~nf.frag.spv
    
    echo generating headers
    %4 %3/%%~nf.vert.spv %3/%%~nf.frag.spv %%~nf %5
)