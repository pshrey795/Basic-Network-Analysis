all:
	g++ traceroute.cpp
	sudo ./a.out $(host)
	sudo ./a.out $(host) > output.csv
	python3 plot.py

clean:
	rm ./a.out && rm output.csv