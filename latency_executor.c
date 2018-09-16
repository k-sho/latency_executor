#include "postgres.h"

#include "executor/instrument.h"
#include "commands/explain.h"

PG_MODULE_MAGIC;

static int		nesting_level = 0;

static ExecutorStart_hook_type prev_ExecutorStart = NULL;
static ExecutorRun_hook_type prev_ExecutorRun = NULL;
static ExecutorFinish_hook_type prev_ExecutorFinish = NULL;
static ExecutorEnd_hook_type prev_ExecutorEnd = NULL;

void _PG_init(void);
void _PG_fini(void);

static void latency_ExecutorStart(QueryDesc *queryDesc, int eflags);
static void latency_ExecutorRun(QueryDesc *queryDesc,
										ScanDirection direction,
										uint64 count, bool execute_one);
static void latency_ExecutorFinish(QueryDesc *queryDesc);
static void latency_ExecutorEnd(QueryDesc *queryDesc);

void
_PG_init(void)
{
	prev_ExecutorStart = ExecutorStart_hook;
	ExecutorStart_hook = latency_ExecutorStart;
	prev_ExecutorRun = ExecutorRun_hook;
	ExecutorRun_hook = latency_ExecutorRun;
	prev_ExecutorFinish = ExecutorFinish_hook;
	ExecutorFinish_hook = latency_ExecutorFinish;
	prev_ExecutorEnd = ExecutorEnd_hook;
	ExecutorEnd_hook = latency_ExecutorEnd;
}

void
_PG_fini(void)
{
	ExecutorStart_hook = prev_ExecutorStart;
	ExecutorRun_hook = prev_ExecutorRun;
	ExecutorFinish_hook = prev_ExecutorFinish;
	ExecutorEnd_hook = prev_ExecutorEnd;
}

static void latency_ExecutorStart(QueryDesc *queryDesc, int eflags)
{
	instr_time start, duration;
	double elapsed_time = 0;

	INSTR_TIME_SET_CURRENT(start);

	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);

	INSTR_TIME_SET_CURRENT(duration);
	INSTR_TIME_SUBTRACT(duration, start);
	elapsed_time = INSTR_TIME_GET_MILLISEC(duration);

	elog(NOTICE, "ExecutorStart %0.3f ms", elapsed_time);
}

static void
latency_ExecutorRun(QueryDesc *queryDesc, ScanDirection direction,
					uint64 count, bool execute_once)
{
	instr_time start, duration;
	double elapsed_time = 0;

	nesting_level++;
	PG_TRY();
	{
		INSTR_TIME_SET_CURRENT(start);
		if (prev_ExecutorRun)
			prev_ExecutorRun(queryDesc, direction, count, execute_once);
		else
			standard_ExecutorRun(queryDesc, direction, count, execute_once);
		
		INSTR_TIME_SET_CURRENT(duration);
		nesting_level--;
	}
	PG_CATCH();
	{
		nesting_level--;
		PG_RE_THROW();
	}
	PG_END_TRY();

	INSTR_TIME_SUBTRACT(duration, start);
	elapsed_time = INSTR_TIME_GET_MILLISEC(duration);

	elog(NOTICE, "ExecutorRun %0.3f ms", elapsed_time);
}

static void
latency_ExecutorFinish(QueryDesc *queryDesc)
{
	instr_time start, duration;
	double elapsed_time = 0;

	nesting_level++;
	PG_TRY();
	{
		INSTR_TIME_SET_CURRENT(start);
		if (prev_ExecutorFinish)
			prev_ExecutorFinish(queryDesc);
		else
			standard_ExecutorFinish(queryDesc);
		
		INSTR_TIME_SET_CURRENT(duration);
		nesting_level--;
	}
	PG_CATCH();
	{
		nesting_level--;
		PG_RE_THROW();
	}
	PG_END_TRY();

	INSTR_TIME_SUBTRACT(duration, start);
	elapsed_time = INSTR_TIME_GET_MILLISEC(duration);

	elog(NOTICE, "ExecutorFinish %0.3f ms", elapsed_time);
}

static void
latency_ExecutorEnd(QueryDesc *queryDesc)
{
	instr_time start, duration;
	double elapsed_time = 0;

	INSTR_TIME_SET_CURRENT(start);
	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
	INSTR_TIME_SET_CURRENT(duration);
	
	INSTR_TIME_SUBTRACT(duration, start);
	elapsed_time = INSTR_TIME_GET_MILLISEC(duration);

	elog(NOTICE, "ExecutorEnd %0.3f ms", elapsed_time);
}
