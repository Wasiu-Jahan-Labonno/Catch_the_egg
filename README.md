

# Open GL (freeGLUT) install in Windows With Visual Studio Code


YouTube Tutorial Guide: [Here](https://www.youtube.com/watch?v=sdTtUm3fay4&ab_channel=ZamanSheikh)
Any problem? [Message in Discord Server](https://discord.gg/Wj3keGKWus) 

Step:

- Download VS Code (https://code.visualstudio.com/download)

- Download MSYS2 (https://www.msys2.org/)

- Generate and Download Glad (https://glad.dav1d.de/) with the latest version(4.6)

- Install VS Code

- Install MSYS2

- Put Glad on MSYS2

- Edit Environment Variables for MSYS2

- Configure VS Code

  - Download Extension(C++)

  - Add IncludePath

MSYS2 MINGW command:

    pacman -Syu
    
    pacman -S mingw-w64-x86_64-toolchain
    
    pacman -S mingw-w64-x86_64-glew
    
    pacman -S mingw-w64-x86_64-glfw
    
    pacman -S mingw-w64-x86_64-freeglut



VS Code Extension

- C/C++

- Code Runner





C/C++ Include path:(Settings)

    ${env:MSYS2}/mingw64/include/



Create Example File minar.cpp [You can use any name]

//Write your openGL code



Run in terminal: `g++ minar.cpp -o minar -lopengl32 -lfreeglut -lglu32 -lglew32 -lglfw3 -lgdi32 -lmingw32`

Execute in terminal: `./minar`


Additional: 
VSCODE Configuration
**Executor Map

    "code-runner.executorMap": {
            "cpp": "cd $dir && g++ $env:MSYS2/mingw64/include/glad/glad.c (Get-ChildItem -Path . -Filter *.cpp -Recurse).FullName -o main -lopengl32 -lfreeglut -lglu32 -lglew32 -lglfw3 -lgdi32 -lmingw32 && ./main"
    },

## Setup in Linux (Any Debian/Ubuntu/Kali) | Just OneClick

1. Open your terminal install those package: 

    `sudo apt install freeglut3-dev mesa-common-dev libxi-dev`
     
    This installs FreeGLUT, OpenGL development libraries, and XInput2 headers.

2. Remove Windows Header:

   - Remove the line #include<windows.h> from your code. It's not needed for Linux.

3. Compile and Run:

   - Save the code as a .cpp file (e.g., myprogram.cpp).
   - Compile it using the following command:

     `g++ myprogram.cpp -o myprogram -lglut -lGLU -lGL`
     

   - Run the executable:
     `./myprogram`

## Setup in Linux (Arch Linux/Any Arch Based Linux) | Just OneClick
1. Install those package

`sudo pacman -S freeglut-devel
sudo pacman -S freeglut
sudo pacman -S glu`

  This installs FreeGLUT, OpenGL development libraries, and XInput2 headers.

2. Remove Windows Header:

   - Remove the line #include<windows.h> from your code. It's not needed for Linux.

2. Compile and Run:

   - Save the code as a .cpp file (e.g., myprogram.cpp).
   - Compile it using the following command:

     `g++ myprogram.cpp -o myprogram -lglut -lGLU -lGL`
     

   - Run the executable:
     `./myprogram`
