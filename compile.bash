#!/bin/bash
clear
if [ ${#} -eq 0 ]
then
    echo 'Incorrect number of arguments' >&2
fi

libs=$()
if [ ${#} -ge 2 ]
then
    for i in ${@:2}
    do
        libs="${libs} -l${i}"
    done
fi

echo "" | cat > Makefile <<- EOM
CXX = g++
LIBS = -pthread \`pkg-config --libs gstreamer-1.0\`
CLIBS = -pthread \`pkg-config --cflags gstreamer-1.0\`
CXXFLAGS = -std=c++17 -fpermissive -Werror -MMD -g \${CLIBS}
EXEC = ${1}
EOM

echo "OBJECTS = " | tr -d '\n' | cat >> Makefile

ls -1 *.cpp | sed -e 's/\..*$//' | sed 's/$/.o /' | tr -d '\n' | cat >> Makefile

TAB="$(printf '\t')"
cat >> Makefile <<- EOM

DEPENDS = \${OBJECTS:.o=.d}

\${EXEC}: \${OBJECTS}
${TAB}\${CXX} \${CXXFLAGS} \${OBJECTS} -o \${EXEC} \${LIBS} ${libs}

-include \${DEPENDS}

.PHONY: clean sweep noMoreExec

clean: 
${TAB}rm \${OBJECTS} \${EXEC} \${DEPENDS}

sweep:
${TAB}rm \${OBJECTS} \${DEPENDS}

noMoreExec:
${TAB}rm \${EXEC}
EOM

# mv ./Makefile ../Makefile
# cd ..
make noMoreExec
make
if [ ${?} -eq 0 ]
then
    make sweep
    clear
    echo "Success!"
fi
# rm Makefile
