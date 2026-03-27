# Basic edge collsion physics simulator program

A very rudimentary implementation to simulate the collision of physics objects with static edges.

## features

- Dynamic object system: objects are handled through data structures and a vector to be able to adjust properties and the number of objects.

- Dynamic static object system: Static objects can be added via a json file that is loaded at runtime

- Collision detection: Physics objects detect collisions with static objects in the scene.

## Roadmap

[ ] Update data structure with new structs to allow for more dynamism.
[ ] Refactor code to smooth out bugs with clipping during collisions.
[ ] Add graphical display of edges/objects added using json file.
