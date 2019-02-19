.PHONY: all

all: chat_client chat_server

% : %.cc
	g++ -I. -I $(BOOST_HOME)/include -L $(BOOST_HOME)/lib -DUNIT_TEST=1 -std=c++14 -g -o $@ $< -lboost_system -lboost_thread -lpthread
