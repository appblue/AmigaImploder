program gwimp;

{$MODE Delphi}

uses
  Forms, Interfaces,
  uMain in 'uMain.pas' {frmMain},
  uFimp in 'uFimp.pas';

{.$R *.res}

{$R *.res}

begin
  Application.Initialize;
  Application.Title := 'WinImploder GUI [by Lab 313]';
  Application.CreateForm(TfrmMain, frmMain);
  Application.Run;
end.
