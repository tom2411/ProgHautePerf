MPICXX = mpic++
SRC= $(wildcard *.cpp)
OBJ= $(SRC:.cpp=.o)
EXEC= main

%.o :%.cpp
	$(MPICXX) -o $@ -c $< -fopenmp

main : $(OBJ) 
	$(MPICXX) -o $@ $^ -fopenmp

.PHONY: clean mrproper

clean:
	@rm -rf *.o

mrproper: clean
	@rm -rf $(EXEC)
