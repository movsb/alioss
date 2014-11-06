md5.o: md5.c md5.h
	$(CC) $(CFLAGS) -o $@ $<

base64_encode.o: base64_encode.c base64_encode.h
	$(CC) $(CFLAGS) -o $@ $<

sha1.o: sha1.c sha1.h
	$(CC) $(CFLAGS) -o $@ $<

hmac_sha1.o: hmac_sha1.c hmac_sha1.h
	$(CC) $(CFLAGS) -o $@ $<

crypto.o: crypto.cpp crypto.h
	$(CXX) $(CXXFLAGS) -o $@ $<

OBJS = md5.o base64_encode.o sha1.o hmac_sha1.o crypto.o

crypto.a: $(OBJS)
	ar cr $@ $^

