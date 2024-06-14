
set compiler=dxc
set options=-Od -Zi ffinite-math-only -remove-unused-functions -remove-unused-globals -Gis -Ges
set options_release=-O3 -ffinite-math-only -remove-unused-functions -remove-unused-globals -Gis -Ges

call %compiler% %options% default.hlsl -Fo default.vert -E mainVS
call %compiler% %options% default.hlsl -Fo default.frag -E mainPS

call %compiler% %options% cubemap.hlsl -Fo cubemap.vert -E mainVS
call %compiler% %options% cubemap.hlsl -Fo cubemap.frag -E mainPS
