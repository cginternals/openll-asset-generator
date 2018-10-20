#!/bin/bash

# Runs the asset generator in docker, passing on all arguments.
# The results are copied into the 'output' (which is created if it doesn't exist)
#
# Usage Example:
# ./run_docker.sh --padding 20 --downsampling 4 --preset ascii --distfield parabola --fontname Arial --fnt

mkdir -p output
docker run -t --rm -v "$(pwd)/output:/output" \
    llassetgen-cmd ./llassetgen-cmd atlas "output/atlas.png" "${@}"

# Hint: for debugging, run this:
# docker run -it llassetgen-cmd bash
