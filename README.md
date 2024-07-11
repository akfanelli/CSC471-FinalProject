# Program 4
## Annie Fanelli
### Controls
* a - rotates the entire scene to the right "camera moves left"
* d - rotates the entire scene to the left "camera moves right"
* w - moves the entire scene backward "camera moves forward"
* s - moves the entire scene forward "camera moves backward"
* m - changes the material of the tree leaves
* e - moves the light to the right
* q - moves the light to the left
* g - activates Bezier curve

I built on top of project 3, while also adding lab 7 and lab 8 to this project. Lab 7 implements the camera movement with WASD and scrollback with the trackpad. Lab 8 is the implementation of the skysphere, which periodically switches back and forth from a day sky and a night sky. 

I added a house mesh that has three different textures applied to the corresponding part of the house.

The last thing that I implemented was the Bezier curve, the fixed point is the cow that is the farthest from the barn. The Bezier curve gives an overhead view of the barn and animals. After the curve is finished you should be able to get a view of the barn, animals and house.

# Program 3
### Controls:
* a - rotates the entire scene to the right "camera moves left"
* d - rotates the entire scene to the left "camera moves right"
* w - moves the entire scene backward "camera moves forward"
* s - moves the entire scene forward "camera moves backward"
* m - changes the material of the tree leaves
* e - moves the light to the right
* q - moves the light to the left
* r - moves the entire scene down "camera moves up"
* f - moves the entire scene up "camera moves down"

I completed implementing lab 5, calculating normals, and lab 6 into this program. After completing these steps, I incorporated the scene that I created in program 2 into this program. The texture mapping mesh is implemented on the ground and the different materials are implmented on eveything else, like the trees which can be toggled by pressing m to change the color of the leaves.

Demonstration of handling a mesh with no normals included in default execution, the mesh with no normals is from one of the provided examples in the base code from bunnyNoNorm.obj, located in the scene at vec3(0, -0.75, 0) and vec3(-2, -0.75, 2).

I tried to get texture mapping working with multiple textures but struggled with the implementation. The only texture that I seem to have gotten to work was setting the ground to grass.jpg. When I tried to set other meshes to different textures, the mesh would either disappear or be a solid color.

I tried to implement fake "projection" shadows for extra credit but was unsuccessful. I tried this by using the bunny mesh and scaled it to make it seem flat but either I just couldn't find the mesh being drawn or this was the wrong way to implement. 
