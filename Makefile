CXX = mpicc
exe = odd_even_sort
obj = odd_even_sort.c

$(exe): $(obj)
        $(CXX) -std=c99 -o $(exe) $(obj)


.PHONY: clean
clean:
        rm $(exe)