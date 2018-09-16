MODULE_big = latency_executor
OBJS = latency_executor.o $(WIN32RES)
PGFILEDESC = "executor_latency - logging execution time"

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
