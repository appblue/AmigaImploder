unit uFimp;

{$MODE Delphi}

interface

type
  PBytesArray = ^TBytesArray;
  TBytesArray = array[0..$800000-1] of Byte;

function implode(input: PBytesArray; len: Integer; mode: Byte): Integer; cdecl; external 'win_imploder.dll' name 'implode';
function explode(input: PBytesArray): Integer; cdecl; external 'win_imploder.dll' name 'explode';
function check_imp(input: PBytesArray): Integer; cdecl; external 'win_imploder.dll' name 'check_imp';
function imploded_size(input: PBytesArray): Integer; cdecl; external 'win_imploder.dll' name 'imploded_size';
function exploded_size(input: PBytesArray): Integer; cdecl; external 'win_imploder.dll' name 'exploded_size';

implementation

end.
