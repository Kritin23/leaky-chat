cd ~/Desktop/ACAD/SEM7/leaky-chat

PHASE_DIR="$(pwd)/.phases/phase2" docker-compose -f attacks/docker/compose.yaml down

PHASE_DIR="$(pwd)/.phases/phase2" docker-compose -f attacks/docker/compose.yaml up -d

docker exec leaky-server cmake -S /workspace/src -B /workspace/build                   
docker exec leaky-server cmake --build /workspace/build -j

docker exec leaky-client1 cmake -S /workspace/src -B /workspace/build
docker exec leaky-client1 cmake --build /workspace/build -j

docker exec leaky-client2 cmake -S /workspace/src -B /workspace/build
docker exec leaky-client2 cmake --build /workspace/build -j

docker exec leaky-mallory cmake -S /workspace/src -B /workspace/build
docker exec leaky-mallory cmake --build /workspace/build -j