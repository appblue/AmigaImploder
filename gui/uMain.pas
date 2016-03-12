unit uMain;

{$MODE Delphi}

interface

uses
  LCLIntf, LCLType, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, Spin, ComCtrls;

type
  TfrmMain = class(TForm)
    edtInput: TLabeledEdit;
    edtOutput: TLabeledEdit;
    btnBrowseIn: TButton;
    btnBrowseOut: TButton;
    mmoLog: TMemo;
    btnRun: TButton;
    grpMode: TGroupBox;
    rbExplode: TRadioButton;
    rbImplode: TRadioButton;
    seImpMode: TSpinEdit;
    dlgOpen: TOpenDialog;
    dlgSave: TSaveDialog;
    rbImplodeTest: TRadioButton;
    rbExplodeAll: TRadioButton;
    edtOffset: TEdit;
    pbProgress: TProgressBar;
    rbImplodedMode: TRadioButton;
    procedure rbExplodeClick(Sender: TObject);
    procedure rbImplodeClick(Sender: TObject);
    procedure edtInputChange(Sender: TObject);
    procedure btnRunClick(Sender: TObject);
    procedure btnBrowseInClick(Sender: TObject);
    procedure btnBrowseOutClick(Sender: TObject);
    procedure rbImplodeTestClick(Sender: TObject);
    procedure edtOffsetKeyPress(Sender: TObject; var Key: Char);
    procedure edtOffsetExit(Sender: TObject);
  private
    { Private declarations }
    function HexToInt(HexStr : string) : Integer;
  public
    { Public declarations }
  end;

var
  frmMain: TfrmMain;

implementation

{$R *.lfm}

uses
  uFimp;

procedure TfrmMain.rbExplodeClick(Sender: TObject);
begin
  seImpMode.Enabled := false;
  edtInputChange(Self);
end;

procedure TfrmMain.rbImplodeClick(Sender: TObject);
begin
  seImpMode.Enabled := true;
  edtInputChange(Self);
end;

procedure TfrmMain.edtInputChange(Sender: TObject);
begin
  btnRun.Enabled := FileExists(edtInput.Text) and
    ((edtOutput.Text <> '') or rbImplodeTest.Checked or rbExplodeAll.Checked or
                               rbImplodedMode.Checked) and
    (edtInput.Text <> edtOutput.Text);
end;

procedure TfrmMain.btnRunClick(Sender: TObject);
var
  inp, outp: TFileStream;
  inbuf, original: PBytesArray;
  inlen, imp_size, outlen, i, min_size, min_i, offset, count: Integer;
  messg, path, save, ifname, ofname: string;
begin
  mmoLog.Clear;
  ifname := edtInput.Text;
  ofname := edtOutput.Text;

  inp := TFileStream.Create(ifname, fmOpenRead, fmShareExclusive);
  New(inbuf);
  New(original);

  inlen := inp.Size;
  inp.Read(original^[0], inlen);
  inp.Free;

  offset := HexToInt(edtOffset.Text);
  if (offset + $30) > inlen then
    offset := 0;

  if rbExplode.Checked then
  begin
    imp_size := imploded_size(@original^[offset]);
    outlen := explode(@original^[offset]);

    if outlen = 0 then
      messg := 'Exploding error!;'
    else
      messg := Format('Output size: %.4X;' + #13#10 + 'Done!', [outlen]);

    mmoLog.Text := Format('Mode: Explosion;' + #13#10 +
                   'Input size: %.4X;' + #13#10 +
                   'Input offset: %.6X;' + #13#10 + messg, [imp_size, offset]);

    if outlen <> 0 then
    begin
      outp := TFileStream.Create(ofname, fmOpenWrite or fmCreate, fmShareExclusive);
      outp.Write(original^[offset], outlen);
      outp.Free;
    end;
  end
  else if rbImplode.Checked then
  begin
    outlen := implode(@original^[0], inlen, Byte(seImpMode.Value));

    if outlen = 0 then
      messg := 'Exploding error!'
    else
      messg := Format('Output size: %.4X;' + #13#10 + 'Done!', [outlen]);

    mmoLog.Text := Format('Mode: Implosion;' + #13#10 +
                   'Input size: %.4X;' + #13#10 + messg, [inlen]);

    if outlen <> 0 then
    begin
      outp := TFileStream.Create(ofname, fmOpenWrite or fmCreate, fmShareExclusive);
      outp.Write(original^[0], outlen);
      outp.Free;
    end;
  end
  else if rbImplodeTest.Checked then
  begin
    mmoLog.Lines.Add('Mode: Implosion test;' + #13#10);

    min_size := MaxInt;
    min_i := 0;
    for i := 0 to 11 do
    begin
      Move(original^, inbuf^, inlen);

      outlen := implode(@inbuf^[0], inlen, Byte(i));

      if (min_size > outlen) and (outlen <> 0) then
      begin
        min_size := outlen;
        min_i := i;
      end;

      mmoLog.Lines.Add(Format('Mode %.2d: %d bytes', [i, outlen]));

      Application.ProcessMessages;
    end;

    mmoLog.Lines.Add(Format(#13#10 + 'Best mode is: %.2d (%.4X bytes).' + #13#10 + 'Done!', [min_i, min_size]));
    seImpMode.Value := min_i;
    rbImplode.Checked := True;
    rbImplodeClick(Self);
  end
  else if rbExplodeAll.Checked then
  begin
    offset := 0;
    count := 0;
    btnRun.Enabled := False;
    pbProgress.Max := inlen;
    path := ExtractFilePath(ifname);

    mmoLog.Lines.Add('Mode: Explode all archives;' + #13#10);


    while (offset + $30) < inlen do
    begin
      pbProgress.Position := offset;
      imp_size := imploded_size(@original^[offset]);

      if (imp_size <> 0) and (offset+imp_size <= inlen) then
      begin
        Move(original^[offset], inbuf^[0], imp_size);
        outlen := explode(@inbuf^[0]);
        if outlen = 0 then
        begin
          Inc(offset);
          Continue;
        end;
        
        save := Format('%.3d_%.6X_exp.bin', [count + 1, offset]);

        outp := TFileStream.Create(path + save, fmOpenWrite or fmCreate, fmShareExclusive);
        outp.Write(inbuf^[0], outlen);
        outp.Free;

        mmoLog.Lines.Add(Format('Exploded at: %.6X (%.4X -> %.4X bytes)', [offset, imp_size, outlen]));
        Inc(count);
      end
      else
        imp_size := 1;

      Inc(offset, imp_size);

      Application.ProcessMessages;
    end;

    mmoLog.Lines.Add(Format(#13#10 + 'Done! Exploded archives count: %d', [count]));
    btnRun.Enabled := True;
  end
  else
  begin
    btnRun.Enabled := False;
    mmoLog.Lines.Add('Mode: Get imploding mode of file;' + #13#10);

    imp_size := imploded_size(@original^[offset]);
    outlen := explode(@original^[offset]);

    if imp_size = 0 then
    begin
      mmoLog.Lines.Add('It''s not an Imploder file!');
    end
    else
    for i := 0 to 11 do
    begin
      Move(original^[offset], inbuf^[0], outlen);

      inlen := implode(@inbuf^[0], outlen, Byte(i));

      seImpMode.Value := i;
      seImpMode.Update;
      if imp_size = inlen then
      begin
        mmoLog.Lines.Add(Format('File mode is: %.2d;' + #13#10 + 'Done!', [i]));
        break;
      end;
      Application.ProcessMessages;
    end;
    btnRun.Enabled := True;
  end;

  Dispose(original);
  Dispose(inbuf);
end;

procedure TfrmMain.btnBrowseInClick(Sender: TObject);
begin
  if not dlgOpen.Execute then Exit;

  edtInput.Text := dlgOpen.FileName;
end;

procedure TfrmMain.btnBrowseOutClick(Sender: TObject);
begin
  if not dlgSave.Execute then Exit;

  edtOutput.Text := dlgSave.FileName;
end;

procedure TfrmMain.rbImplodeTestClick(Sender: TObject);
begin
  seImpMode.Enabled := false;
  edtInputChange(Self);
end;

procedure TfrmMain.edtOffsetKeyPress(Sender: TObject; var Key: Char);
begin
  if not (Key in ['0'..'9', 'A'..'F', 'a'..'f', #$2E, #$08]) then Key := #0;
end;

function TfrmMain.HexToInt(HexStr : string) : Integer;
var RetVar : Integer;
    i : byte;
begin
  if Length(HexStr) = 0 then
  begin
    Result := 0;
    Exit;
  end;

  HexStr := UpperCase(HexStr);
  if HexStr[length(HexStr)] = 'H' then
     Delete(HexStr,length(HexStr),1);
  RetVar := 0;
   
  for i := 1 to length(HexStr) do begin
      RetVar := RetVar shl 4;
      if (HexStr[i] in ['0'..'9']) then
         RetVar := RetVar + (byte(HexStr[i]) - 48)
      else
         if (HexStr[i] in ['A'..'F']) then
            RetVar := RetVar + (byte(HexStr[i]) - 55)
         else begin
            Retvar := 0;
            break;
         end;
  end;
   
  Result := RetVar;
end;

procedure TfrmMain.edtOffsetExit(Sender: TObject);
begin
  edtOffset.Text := Format('%.6X', [HexToInt(edtOffset.Text)]);
end;

end.
