cd src
#g++ *.cpp -c  -std=c++11 -Wall -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT   -g    -I/usr/include/OpenEXR
#g++ *.o -o trinity -lSDL -lpthread  -lSDL_net  -lIlmImf -lHalf -lIex
g++ *.cpp -c  -std=c++11 -Wall -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT   -O3  -ffast-math    -I/usr/include/OpenEXR 
g++ *.o -o trinity -lSDL -lpthread  -s -ffast-math -lSDL_net  -lIlmImf -lHalf -lIex
rm *.o
mv trinity ..
cd ..