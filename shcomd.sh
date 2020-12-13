set -e

for file in $1/*.vert; do
    shader=${file%.vert}
    shader=${shader##*/}
    echo generating shader $shader

    echo compiling $shader.vert
    glslc $file -o $2/$shader.vert.spv
    echo $shader.vert written to $2/$shader.vert.spv

    echo compiling $shader.frag
    glslc ${file%.vert}.frag -o $2/$shader.frag.spv
    echo $shader.frag written to $2/$shader.frag.spv

    echo launching pcsr for $shader
    $3 $2/$shader.vert.spv $2/$shader.frag.spv $shader $4
    echo $shader generated
done
