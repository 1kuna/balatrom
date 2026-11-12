using System;using System.Linq;using System.Threading;using System.Threading.Tasks;
class BusyStateTests {
 static void Need(bool v,string m){if(!v)throw new Exception(m);}
 static int Main(){
  int passed=0;Action<string>pass=s=>{Console.WriteLine("PASS "+s);passed++;};
  var a=new SetupRunState();Need(a.TryStart(),"first activation refused");int generation=a.Generation;Need(a.IsRunning,"not busy synchronously");Need(!a.TryStart(),"double click admitted");Need(!a.TryStart(),"keyboard/reentry admitted");pass("first activation synchronously holds gate across mouse, keyboard and reentry");
  Need(a.CanApplyProgress(generation),"active progress denied");Need(!a.TryStart(),"retry before teardown admitted");a.FinishAfterTeardown(false,false);Need(!a.CanApplyProgress(generation),"late progress admitted after worker teardown");Need(a.TryStart(),"safe explicit retry denied");Need(a.Generation!=generation&&!a.CanApplyProgress(generation),"old progress updates new run");pass("failure only reopens gate after teardown and rejects old queued progress");
  a.FinishAfterTeardown(true,false);Need(!a.TryStart(),"second launch after success admitted");pass("successful launch stays terminal even with a queued activation");
  var b=new SetupRunState();b.TryStart();b.FinishAfterTeardown(false,true);Need(!b.TryStart(),"closing worker reopened");pass("closing/cancellation stays terminal after teardown");
  var c=new SetupRunState();int admitted=0;Parallel.For(0,32,i=>{if(c.TryStart())Interlocked.Increment(ref admitted);});Need(admitted==1,"parallel duplicate setup admitted");pass("32 simultaneous activation attempts admit exactly one worker");
  var d=new SetupRunState();Need(!d.HasStarted,"unexpected startup generation");d.TryStart();d.FinishAfterTeardown(false,false);Need(d.HasStarted,"startup probe can accidentally retry a failed user activation");pass("startup readiness probe cannot auto-retry after any user attempt");
  Console.WriteLine("RESULT "+passed+" runtime gate checks passed; WinForms rendering/native Windows not executed");return 0;
 }
}
