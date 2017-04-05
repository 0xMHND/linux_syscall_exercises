CC = icc
CXX = icpc
CFLAGS = -Wall -std=c99 -openmp -O3 -g -I.
CXXFLAGS = -Wall -openmp -O3 -g
#overkill on the flags, but that doesn't hurt anything
LDFLAGS = -lrt -lpthread 
CTARGET = msh
CSOURCE = msh.c
CINCLUDES = 
#Target for the tex file
TARGET=msh



#default is to compile and create the pdf
default: compile

all: compile pdf

clean: cclean clean-pdf




#~~~~~~~~~~~~~~~~~~~~ C ~~~~~~~~~~~~~~~~~~~~#
#depends on all of you source and header files
compile: ${CSOURCE} ${CINCLUDES}
#this assumes you actually are linking all of the source files together
	${CC} ${CFLAGS} ${CSOURCE} -o ${CTARGET} ${LDFLAGS}

debug: ${CSOURCE} ${CINCLUDES}
	${CC} ${CFLAGS} ${CSOURCE} -o ${CTARGET} ${LDFLAGS} -DDEBUG

cclean:
	rm -f ${CTARGET}




#~~~~~~~~~~~~~~~~~~~ Tex ~~~~~~~~~~~~~~~~~~~#
dvi: ${TARGET}.tex 
	latex ${TARGET}.tex
	latex $(TARGET).tex

ps: dvi
	dvips -R -Poutline -t letter ${TARGET}.dvi -o ${TARGET}.ps

pdf: ps
	ps2pdf ${TARGET}.ps

clean-dvi:
	rm -f ${TARGET}.aux
	rm -f ${TARGET}.log
	rm -f ${TARGET}.dvi
	rm -f ${TARGET}.out

clean-ps: clean-dvi
	rm -f ${TARGET}.ps

clean-pdf: clean-ps
	rm -f ${TARGET}.pdf
