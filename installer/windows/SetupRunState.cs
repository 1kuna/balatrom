using System;
using System.Threading;
// Setup run state: retry is allowed only after teardown.
internal sealed class SetupRunState {
 const int Idle=0,Running=1,Succeeded=2,Closed=3;
 int state,generation;
 public bool IsRunning {get{return Volatile.Read(ref state)==Running;}}
 public bool HasStarted {get{return Volatile.Read(ref generation)!=0;}}
 public int Generation {get{return Volatile.Read(ref generation);}}
 public bool TryStart(){if(Interlocked.CompareExchange(ref state,Running,Idle)!=Idle)return false;Interlocked.Increment(ref generation);return true;}
 public bool CanApplyProgress(int run){return IsRunning&&Generation==run;}
 public void FinishAfterTeardown(bool succeeded,bool closing){if(!IsRunning)throw new InvalidOperationException("No setup worker is running.");Volatile.Write(ref state,succeeded?Succeeded:closing?Closed:Idle);}
}
