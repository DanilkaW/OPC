//----------------------------------------------------------------------------
#pragma hdrstop
#include "Master.h"
#include <vcl.h>
#include "TestFormOPC.h"
#include "ReadThread.h"
//----------------------------------------------------------------------------
#pragma package(smart_init)

//----------------------------------------------------------------------------
Master *MyMaster;
WriteThread *ThreadObjectName[9];
extern ThreadConf MyThreadConfig;

//----------------------------------------------------------------------------
AnsiString FileName;
AnsiString FileNC = "D:\\USER\\OPC_Projects\\project.config";
int hFile, hFileConf;
bool COMPortNeed;

//----------------------------------------------------------------------------
void __fastcall Master::ReadStruct(void)        //Чтение и заполнение структуры из файла
{
        int tmp = 19;   //Сдвиг в файле на позицию "полезности" порта
        //---
        hFile = FileOpen(FileName, 0x0);
        //---
        for(int i = 0; i < 9; i++){     //Заполнение структуры
                FileSeek(hFile, tmp, 0);
                FileRead(hFile, &COMPortNeed, sizeof(bool));
                if(COMPortNeed){
                        if(!GetCommPort(i)){
                                ShowMessage("Порт открыть не удалось, обратитесь к разработчику");
                                return ;
                        }
                        else{
                                FileSeek(hFile, tmp - 10, 0);
                                FileRead(hFile, &MyThreadConfig.COMSpeed[i], sizeof(int));
                                FileSeek(hFile, tmp - 4, 0);
                                FileRead(hFile, &MyThreadConfig.COMSensorName[i], sizeof(us_int));
                                FileSeek(hFile, tmp - 2, 0);
                                FileRead(hFile, &MyThreadConfig.COMSensorAmount[i], sizeof(us_int));
                                int tmp2 = tmp + 1;     //Сдвиг в файле на позицию "полезности" датчика
                                for(int SensCount = 0; SensCount < 8; SensCount++){
                                        FileSeek(hFile, tmp2, 0);
                                        FileRead(hFile, &MyThreadConfig.COMSensor[i][SensCount], sizeof(bool));
                                        tmp2++;
                                }
                        }
                }
                tmp += 19;
        }
        FileClose(hFile);
        //---
        for(int i = 0; i < 9; i++){     //Зануление массива Handle'ов потоков
                MyThreadConfig.HandleThread[i] = 0;
                MyThreadConfig.HandleFile[i] = 0;
        }
        //---
        for(int COMNumber = 0; COMNumber < 9; COMNumber++){
                if(MyThreadConfig.COMSensorName[COMNumber] != 0){
                        SwitchThread(COMNumber);
                }
        }
}
//---------------------------------------------------------------------------- 
void __fastcall Master::SwitchThread(int COMNumber)     //Открытие потоков отправки
{
        ThreadObjectName[COMNumber] = new WriteThread(true);
        ThreadObjectName[COMNumber]->COMName = COMNumber;
        ThreadObjectName[COMNumber]->PortHandle = OpenPort(COMNumber);
        ThreadObjectName[COMNumber]->FreeOnTerminate = true;
        ThreadObjectName[COMNumber]->Resume(); 
}
//----------------------------------------------------------------------------
void __fastcall Master::OpenProj(void)  //Открытие проекта
{
        ServerForm->OpenProject->InitialDir = "D:\\USER\\OPC_Projects";
        ServerForm->OpenProject->Filter = "Проекты OPC (*.prj)|*.prj";
        ServerForm->OpenProject->Execute();
        //---
        if(ServerForm->OpenProject->FileName <= 0){
                ShowMessage("   Проект не был открыт   ");
                ServerForm->StateProject = 0;
                return ;
        }
        FileName = ServerForm->OpenProject->FileName;
        ServerForm->StateProject = 1;
        //---
        int lenAn = FileName.Length();
        FileWrite(hFileConf, &lenAn, sizeof(int));
        char *Stmp;
        Stmp = FileName.c_str();
        FileSeek(hFileConf, 4, 0);
        FileWrite(hFileConf, &Stmp, sizeof(char)*lenAn);
        //---
        ReadStruct();
//----------------------------------------------------------------------------
bool __fastcall Master::GetCommPort(int i)      //Проверка доступности COM-порта
{
        HANDLE CheckPort;
        char sPortName[5] = "COM";
        char tmp[2];
        //---
        itoa(i + 1, tmp, 10);
        strcat(sPortName, tmp);
        CheckPort = ::CreateFile(sPortName,
                               GENERIC_READ | GENERIC_WRITE,    
                               0,
                               0,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               0);      //0x80000000L | 0x40000000L
        CloseHandle(CheckPort);
        if(CheckPort == INVALID_HANDLE_VALUE){
                return false;
        }
        return true;
}
//----------------------------------------------------------------------------
void __fastcall Master::CloseAndDelete(void)    //Закрытие потоков и удаление сопутствующих файлов
{
        for(int i = 0; i < 9; i++){
                if(MyThreadConfig.HandleFile[i] > 0){
                        FileClose(MyThreadConfig.HandleFile[i]);
                        AnsiString FileNamePaste = "D:\\USER\\OPC_Projects\\" + IntToStr(MyThreadConfig.HandleFile[i]) + ".tfg";
                        FileSetAttr(FileNamePaste, 0);
                        DeleteFile(FileNamePaste);
                }
        }
}
//----------------------------------------------------------------------------
void __fastcall Master::CheckProject(void)      //Автоматическое открытие последнего проекта (проверить)
{
        hFileConf = FileOpen(FileNC, 0x2);
        //---
        int lenAn = 0;
        FileRead(hFileConf, &lenAn, sizeof(int));
        if(lenAn <= 0){
                ServerForm->StateProject = 0;
                return ;
        }
        //char *Stmp = new char[lenAn];
        FileSeek(hFileConf, 4, 0);
        FileRead(hFileConf, &Stmp, sizeof(char)*lenAn);
        //FileName = Stmp;
        if(FileName.IsEmpty()){
                ServerForm->StateProject = 0;
                return ;
        }
        ServerForm->StateProject = 1;
}
