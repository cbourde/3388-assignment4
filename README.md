# CS3388 Assignment 4
## Submitted Files
Everything is within a zip file to preserve the directory structure.
- `as4.cpp`: Source code
- `assets/*.ply`: Mesh files
- `assets/*.bmp`: Texture files
- `screenshot*.png`: Screenshots of program operation (from 4 different angles)

## Compiling and running
Unzip and don't change the directory structure. The compilation command should be as follows, assuming you're in the same directory as `as4.cpp`:  
`g++ -g as4.cpp -o as4 -lGL -lglfw -lGLEW`  
Then run the resulting `as4` binary.

## Configuration
The screen size, FOV, and movement/rotation speed are all near the top of `as4.cpp` if you want to mess around with them.

## Known bugs
The `TexturedMesh` constructor doesn't actually check whether reading the texture or PLY files worked, so passing in a bad path or incorrectly formatted file will probably screw things up.

## Code explanation
### Data structures/Classes
- `VertexData`: contains information about a vertex (position, normals, colour, and texture coordinates). The normals and colour aren't needed for this assignment, but the instructions mentioned them so I included them on the off chance that future assignments might allow me to reuse or extend this assignment's code.
- `TriData`: represents a single triangle. Contains three integers representing indices into a vertex buffer.
- `TexturedMesh`: Represents a textured triangle mesh. Contains a list of `VertexData` and a list of `TriData`, which are both read from a PLY file on instantiation. Contains a pointer to the texture data, which is read from a BMP file on instantiation. Contains IDs for a VAO, various VBOs, a texture object, and a shader program, which are created on instantiation and used in the `draw()` function.

### Functions
- `main`: First initializes the window and GLEW, then creates all of the `TexturedMesh` objects using the files in the `assets` directory. Initializes OpenGL states (depth testing and background colour) and the camera position and direction. Enters a main loop which moves the camera based on keyboard input, then draws all of the `TexturedMesh` objects, repeating until the window is closed.
- `loadPLY(path, vertices, faces)`: Reads mesh data from a PLY file. Operation is as follows:
	1. Open the file from `path` and make sure it's valid by checking that the first line is "ply"
	2. Read the header line by line:
		- Ignore lines starting with "comment", "format", or "property" since the files are all in ASCII and all have the properties in the same order.
		- For lines starting with "element", save the number of elements since that will be needed when actually reading the vertex and face data. (if the word after "element" isn't "vertex" or "face", then the file is bad)
		- When the "end_header" line is reached, we're done. If there wasn't a vertex or face count, the file is bad.
	3. Read the vertex data (a number of lines equal to the vertex count from the header). Each line is 8 floats that go into a vector. If there were actually 8 values and they were all floats, put the entries from the vector into a new `VertexData` and push it to the `vertices` list.
	4. Read the face data (a number of lines equal to the face count from the header). Each is 4 integers, with the first being the number of values following it. This should always be 3, but I checked it against the number of indices actually read anyway just to be safe. If there were at least 3 indices, create a new `TriData` from the first 3 and push it to the `faces` list.
- `loadARGB_BMP(path, data, width, height)`: Reads the data from the BMP file at `path` into the `data` pointer. This code was provided with the assignment instructions, but I copied it into the main source file because I didn't feel like figuring out how multi-file programs work.
- `TexturedMesh::TexturedMesh(PLY_path, tex_path)`: Constructor for TexturedMesh. Operation is as follows:
	1. Read the PLY and BMP files into the appropriate vectors and data pointer.
	2. Create and bind the VAO.
	3. Create the VBO for vertex positions from the `vertices` vector. The attribute pointer uses a stride value of 11 * sizeof(float) since that's the size of a `VertexData` struct.
	4. Create the VBO for texture coordinates. Uses the same `vertices` vector and stride value, but starts at an offset of 9 * sizeof(float) since that's where the texture coordinates are in a `VertexData` struct.
	5. Create the VBO for vertex indices from the `faces` vector. This doesn't need an attribute pointer since it's not used by the shaders.
	6. Unbind the VAO since it's the best practice.
	7. Create the shader program. The vertex and fragment shaders are shamelessly stolen from class demo code, as instructed. The shader sources and compiled individual shaders are detached and deleted after the program is linked since that's the best bractice.
	8. Create the texture object and pass it the data read from the BMP file. It uses the BGRA format (although using RGBA makes everything blue which is kind of neat) and the width and height which were loaded from the BMP by `loadARGB_BMP` earlier.
	9. Unbind the texture object since that's the best practice.
- `TexturedMesh::draw(mvp)`: Renders a `TexturedMesh` object. Operation is as follows:
	1. Set the active texture unit to the one created in the constructor, and enable blending.
	2. Set the active shader program to the one created in the constructor. Get the ID for the shader program's uniform MVP matrix and set it to the matrix passed in as `mvp`.
	3. Bind the VAO.
	4. Use `glDrawElements` to draw all of the triangles. Since the array of indices was passed into `GL_ELEMENT_ARRAY_BUFFER` as part of creating the VAO, the pointer for `glDrawElements` can just be zero instead of a pointer to the `faces` vector.
	5. Disable the shader program and unbind the VAO and texture object (best practices).