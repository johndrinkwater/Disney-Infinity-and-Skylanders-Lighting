all: infinitylightsbin lightshowbin

infinitylightsbin:
	rm -f infinitylights
	g++ main.cpp InfinityBase.cpp SkylandersPortal.cpp -o infinitylights -lusb-1.0

lightshowbin:
	rm -f lightshow
	g++ lightshow.cpp InfinityBase.cpp SkylandersPortal.cpp -o lightshow -lusb-1.0

clean:
	rm -f infinitylights lightshow
