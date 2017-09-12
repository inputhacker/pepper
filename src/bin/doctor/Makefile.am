bin_PROGRAMS =

bin_PROGRAMS += doctor doctor-client

doctor_CFLAGS = $(DOCTOR_SERVER_CFLAGS)
doctor_LDADD  = $(DOCTOR_SERVER_LIBS)

doctor_SOURCES = doctor.c

doctor_client_CFLAGS = $(DOCTOR_CLIENT_CFLAGS)
doctor_client_LDADD = $(DOCTOR_CLIENT_LIBS)

doctor_client_SOURCES = doctor-client.c

