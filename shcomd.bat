@echo OFF
rem %1 - processing dir %2 spir-v dir %3 pcsr path %4 out header dir

for %%f in (%1/*.vert) do (
    echo generating shader %%~nf

    echo compiling %%~nf.vert
    glslc.exe %1/%%~nf.vert -o %2/%%~nf.vert.spv || exit /b
    echo %%~nf.vert written to %2/%%~nf.vert.spv

    echo compiling %%~nf.frag
    glslc.exe %1/%%~nf.frag -o %2/%%~nf.frag.spv || exit /b
    echo %%~nf.frag written to %2/%%~nf.frag.spv

    echo launching pcsr for %%~nf
    %3 %2/%%~nf.vert.spv %2/%%~nf.frag.spv %%~nf %4 || exit /b
    echo %%~nf generated
)
