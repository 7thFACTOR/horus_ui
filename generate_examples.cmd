md build
cd ./build
cmake -G "Visual Studio 17 2022"  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DOpenGL_GL_PREFERENCE=GLVND ../examples
cd ..