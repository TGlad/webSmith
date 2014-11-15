#if 0
class Thread
{
public:
  Thread();
  int Start(void * arg);
protected:
  int Run(void * arg);
  static void * EntryPoint(void*);
  virtual void Setup();
  virtual void Execute(void*);
  void * Arg() const {return Arg_;}
  void Arg(void* a){Arg_ = a;}
private:
  THREADID ThreadId_;
  void * Arg_;
};

Thread::Thread() {}

int Thread::Start(void * arg)
{
  Arg(arg); // store user data
  int code = thread_create(Thread::EntryPoint, this, & ThreadId_);
  return code;
}

int Thread::Run(void * arg)
{
  Setup();
  Execute( arg );
}

/*static */
void * Thread::EntryPoint(void * pthis)
{
  Thread * pt = (Thread*)pthis;
  pthis->Run( Arg() );
}

virtual void Thread::Setup()
{
  // Do any setup here
}

virtual void Thread::Execute(void* arg)
{
  // Your code goes here
}


#endif




class CThread
{
public:
  /*
  *   Info: Default Constructor
  */
  CThread();

  /*
  *   Info: Plug Constructor
  */
  CThread(LPTHREAD_START_ROUTINE lpExternalRoutine);

  /*
  *   Info: Default Destructor
  */
  ~CThread();

  /*
  *   Info: Starts the thread.
  */
  DWORD Start( void* arg = NULL );

  /*
  *   Info: Stops the thread.
  */
  DWORD Stop ( bool bForceKill = false );

  /*
  *   Info: Starts the thread.
  */
  DWORD GetExitCode() const;

  /*
  *   Info: Attaches a Thread Function
  */
  void Attach( LPTHREAD_START_ROUTINE lpThreadFunc );

  /*
  *   Info: Detaches the Attached Thread Function
  */
  void  Detach( void );

protected:

  /*
  *   Info: DONT override this method.
  */
  static DWORD WINAPI EntryPoint( LPVOID pArg);

  /*
  *   Info: Override this method.
  */
  virtual DWORD Run( LPVOID arg );

  /*
  *   Info: Constructor-like function. 
  */
  virtual void ThreadCtor();

  /*
  *   Info: Destructor-like function. 
  */
  virtual void ThreadDtor();

private:
  /*
  *   Info: Thread Context Inner Class
  *
  *   Every thread object needs to be associated with a set 
  *   of values like UserData Pointer, Handle, Thread ID etc.
  *  
  *  NOTE: This class can be enhanced to varying functionalities
  *    eg.,
  *    * Members to hold StackSize
  *    * SECURITY_ATTRIBUTES member.
  */
  class CThreadContext
  {
  public:
    CThreadContext();

    /*
    *   Attributes Section
    */
  public:
    HANDLE m_hThread;     // The Thread Handle
    DWORD  m_dwTID;       // The Thread ID
    LPVOID m_pUserData;   // The user data pointer
    LPVOID m_pParent;     // The this pointer of the parent
    //   CThread object
    DWORD  m_dwExitCode;  // The Exit Code of the thread
  };

  /*
  *   Attributes Section
  */
protected:
  /*
  *   Info: Members of CThread
  */
  CThreadContext  m_ThreadCtx; // The Thread Context member
  LPTHREAD_START_ROUTINE m_pThreadFunc; // The Worker Thread
  // Function Pointer
};