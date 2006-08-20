/* Cleanup XWorkplace */
signal on halt
parse upper arg y2all .
'@echo off'

/* Compiled XWorkplace files */
say 'Do you wish to delete all .OBJ, .RES, .EXE, .DLL, .MAP, .SYM and .TMF files in'
call charout , 'the source directories [Y/N] ? '
if y2all = 'Y' then say 'Y'; else parse upper linein yn .
if y2all = 'Y' | yn = 'Y' then do
  call deletefiles 'bin\*.obj'
  call deletefiles 'bin\*.res'
  call deletefiles 'bin\dll_mt\*.obj'
  call deletefiles 'bin\dll_mt\*.res'
  call deletefiles 'bin\dll_subs\*.obj'
  call deletefiles 'bin\exe_mt\*.obj'
  call deletefiles 'bin\exe_mt\*.res'
  call deletefiles 'bin\exe_st\*.obj'
  call deletefiles 'bin\modules\*.exe'
  call deletefiles 'bin\modules\*.dll'
  call deletefiles 'bin\modules\*.map'
  call deletefiles 'bin\modules\*.sym'
  call deletefiles 'bin\modules\*.tmf'
  call deletefiles 'bin\widgets\*.obj'
end

/* LIB files */
call charout , 'Do you wish to delete the LIB files [Y/N] ? '
if y2all = 'Y' then say 'Y'; else parse upper linein yn .
if y2all = 'Y' | yn = 'Y' then do
  call deletefiles 'bin\*.lib'
  call deletefiles 'bin\exe_mt\*.lib'
end

/* INF/HLP files */
call charout , 'Do you wish to delete the INF/HLP files [Y/N] ? '
if y2all = 'Y' then say 'Y'; else parse upper linein yn .
if y2all = 'Y' | yn = 'Y' then do
  call deletefiles 'bin\modules\*.inf'
  call deletefiles 'bin\modules\*.hlp'
  call deletefiles 'bin\modules\*.tmf'
end

/* IPF source files */
call charout , 'Do you wish to delete the IPF source files [Y/N] ? '
if y2all = 'Y' then say 'Y'; else parse upper linein yn .
if y2all = 'Y' | yn = 'Y' then do
  call deletefiles 'bin\modules\*.ipf'
  call deletefiles '001\inf.001\*.bmp'
  call deletefiles '049_de\inf.049\*.bmp'
  call deletefiles '001\xwphelp2\img\*.bmp'
end

/* SOM headers */
call charout , 'Do you wish to delete all the SC-created .DEF, .IH and .H files [Y/N] ? '
if y2all = 'Y' then say 'Y'; else parse upper linein yn .
if y2all = 'Y' | yn = 'Y' then do
  call deletefiles 'idl\*.def'
  call deletefiles 'include\classes\*.h'
  call deletefiles 'include\classes\*.ih'
  call deletefiles 'include\classes\wps\*.h'
  call deletefiles 'include\classes\wps\*.ih'
end
return

halt:
  say
  return

deletefiles: procedure
  arg name
  'if exist' name 'del' name
  return
