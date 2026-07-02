class KeyInterpreter : public vtkCommand 
{
public:
    // Allocator
    static KeyInterpreter *New( void ) { return new KeyInterpreter; }

    // Internal data.  This needs to be set immediately after allocation.
    vtkVolumeRayCastMapper *map;

    // Called upon the registered event (i.e., a key press)
    void Execute( vtkObject* caller, unsigned long eventId, void *callData )
    {
        double dist;
        vtkRenderWindowInteractor *iren = 
                    reinterpret_cast<vtkRenderWindowInteractor *>(caller);
        switch( iren->GetKeyCode() )
        {
            case '+':
            case '=':
                dist = map->GetSampleDistance();
                
				// Do something here

                map->SetSampleDistance( dist );
                break;
            case '-':
            case '_':
                dist = map->GetSampleDistance();
                
				// Do something here

                map->SetSampleDistance( dist );
                break;
        }
        iren->Render();
    }
};



// To register the keyboard callback
KeyInterpreter *key = KeyInterpreter::New();
key->map = volumeMapper;
iren->AddObserver(vtkCommand::KeyPressEvent, key );
