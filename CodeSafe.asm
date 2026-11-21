INCLUDE Irvine32.inc



GetStringLength PROTO C :DWORD
ReverseString   PROTO C :DWORD

.data

; for random generator
isSeed Byte 0

TotalQ EQU 3
quizscore DWORD 0

;*********************************
;Quiz options and answer 

q1  BYTE "Q1: Min length required for a Strong password?", 0
q1a  BYTE "1) 4 characters", 0
q1b     BYTE "2) 6 characters", 0
q1c BYTE "3) 8 characters", 0
q1d BYTE "4) 12 characters", 0



q2  BYTE "Q2: What is the valid Shift Key range in Crypt Module?", 0
q2a  BYTE "1) 1 to 25", 0
q2b  BYTE "2) 0 to 100", 0
q2c     BYTE "3) 1 to 10", 0
q2d     BYTE "4) 5 to 50", 0

q3  BYTE "Q3: How many digits are in a generated Secret Code?", 0
q3a     BYTE "1) 2 digits", 0
q3b   BYTE "2) 3 digits", 0
q3c     BYTE "3) 4 digits", 0
q3d     BYTE "4) 6 digits", 0

emptyStr Byte 0

qTextTable Dword offset q1, Offset q2,OFFSET q3

qOptTable  DWORD OFFSET q1a,OFFSET q1b,OFFSET q1c ,OFFSET q1d
           DWORD OFFSET q2a, OFFSET q2b, OFFSET q2c,OFFSET q2d
           DWORD OFFSET q3a, OFFSET q3b,OFFSET q3c,OFFSET q3d

ansTable   BYTE 3, 1, 3




.code

;************************
    ;1st helper for String Length

GetStringLength PROC C pText:DWORD
    push esi
    mov esi, pText
    xor eax, eax            ; length counter = 0

gsl_loop:
    mov cl, byte ptr [esi]  ; FIXED syntax
    test cl, cl             ; check for null
    jz gsldone              ; if null
    inc eax
    inc esi
    jmp gsl_loop

gsldone:
    pop esi
    ret
GetStringLength ENDP

;*********************************
;2nd Helper for Reverse String

ReverseString PROC C pText:DWORD
    push ebx
    push esi
    push edi

    push pText
    call GetStringLength    ; matched case
    add esp, 4

    test eax, eax
    jz rsdone               ; if length 0 do nothing

    mov esi, pText          ; string start
    mov edi, pText
    add edi, eax
    dec edi                 ; end

rsloop:
    cmp esi, edi
    jae rsdone              ; if start >= end we stop

    ; swap characters
    mov al, byte ptr [esi]
    mov bl, byte ptr [edi]
    mov byte ptr [esi], bl
    mov byte ptr [edi], al

    inc esi                 ; move forward
    dec edi                 ; move backward
    jmp rsloop

rsdone:
    pop edi
    pop esi
    pop ebx
    ret
ReverseString ENDP          

;**********************
    ; Crypt string using Caesar cipher
    ; shifts letters and numbers (1-25), 0 = encrypt, 1 = decrypt

CryptString PROC C pText:DWORD, shiftBy:DWORD, decMode:DWORD
    push ebx
    push esi

    mov esi, pText          
    mov ebx,shiftBy         

cs_loop:
    mov al, byte ptr [esi]
    test al, al
    jz cs_done              ; exit if null

    ;1. identify char type: DL = ascii base, AH = modulo limit


    cmp al,'0'
    jb csnext               ; skip spaces and symbols
    cmp al,'9'
    jbe is_num

    cmp al, 'A'
    jb csnext
    cmp al, 'Z'
    jbe is_upper

    cmp al, 'a'
    jb csnext
    cmp al, 'z'
    ja csnext

is_lower:
    mov dl, 'a'
    mov ah, 26
    jmp doshift

is_upper:
    mov dl,'A'
    mov ah,26
    jmp doshift

is_num:
    mov dl,'0'
    mov ah, 10

doshift:
    sub al, dl              ; 2. strip the ascii base
    cmp decMode, 1
    je dodecrypt            ; jmp if decrypting

doencrypt:
    add al, bl              ; add shift

enc_wrap:
    cmp al, ah              ; past modulo (26 or 10)?
    jl apply_char
    sub al, ah
    jmp enc_wrap            ; FIXED label

dodecrypt:
    sub al, bl              ; subtract shift

dec_wrap:
    cmp al, 0               ; below 0?
    jge apply_char
    add al, ah
    jmp dec_wrap            ; FIXED label

apply_char:
    add al, dl              ; 3. add the ascii base back
    mov byte ptr [esi], al

csnext:
    inc esi
    jmp cs_loop

cs_done:                    ; FIXED label
    pop esi
    pop ebx
    ret
CryptString ENDP            ; FIXED case

;**************
    ; Check password strength
    ; returns 0 = Weak, 1 = Medium, 2 = Strong

CheckPasswordStrength PROC C pText:DWORD   ;

    push ebx
    push esi
    mov esi, pText
    xor ecx, ecx            ; ECX = length counter
    xor ebx, ebx            ; EBX = bit flags (Bit 0=Up, 1=Low, 2=Dig, 3=Spec)

    chkloop:
    mov al,[esi]
    test al,al
    jz chkL              
    inc ecx

    ;1) Digit
    cmp al,'0'
    jb chkspecial
    cmp al,'9'
    jbe isdigit1

    ;2)upper case

    cmp al, 'A'
    jb chkspecial
    cmp al,'Z'
    jbe isupper

    ;3)lower case
    
    cmp al, 'a'
    jb chkspecial
    cmp al,'z'
    jbe is_lower

chkspecial:
    or ebx, 8                       ;bit 3 = special char
    jmp chk_next

isdigit1 :
    or ebx, 4                       ;bit 2 = digit
    jmp chk_next

isupper:
    or ebx,1                       ;bit 0 = upper
    jmp chk_next

is_lower:
    or ebx, 2                       ;lowercase bit 1

chk_next:
    inc esi
    jmp chkloop

chkL:
    cmp ecx, 8                       ; length >= 8?
    jb chkweak

    ;count num of char
    xor eax,eax

    mov edx,ebx
    and edx,1
    add eax,edx                       ;upper

    mov edx,ebx
    shr edx,1
    and edx,1
    add eax,edx                       ;lower

    mov edx,ebx
    shr edx,2
    and edx,1
    add eax,edx                       ;digit

    mov edx,ebx 
    shr edx,3
    and edx,1
    add eax,edx                       ;special

    cmp eax,3
    jge checkstrong         ; 3 or 4 types

chkweak:
    mov eax,0           ; weak
    jmp chkdone ;

checkstrong:
mov eax,1 ; strong

chkdone:
ret

CheckPasswordStrength EndP

;*************************************
;      Generatecode using random range 

    
GenerateCode proc C Seed:DWORD
    mov eax, Seed
    cmp isSeed ,0
    jne genNum
    call  Randomize
    mov isSeed,1

genNum :
    mov eax,9000
    call RandomRange
    add eax,1000
    ret

GenerateCode endP




;************************************
;PROC for QUIZ 

ResetQuiz PROC C
    mov quizscore, 0
    ret
ResetQuiz EndP

GetTotalQuestions Proc C
    mov eax, totalQ
    ret
GetTotalQuestions EndP

GetQuizScore Proc C
    mov eax,quizScore
    ret
GetQuizScore ENDP

GetQuizQuestion Proc C, qIndex:Dword
    mov eax, qIndex
    cmp eax,totalQ
    jae gqqinvalid
    mov eax,qTextTable[eax*4]
    ret

gqqinvalid:
    mov eax,Offset emptyStr
    ret
GetQuizQuestion ENDP

GetQuizOption PROC C uses ebx,qindex:Dword, optIndex:DWORD
    mov eax, qindex
    cmp eax,TotalQ
    jae GQOinvalid
    mov ebx,optindex
    cmp ebx,4
    jae gqoinvalid

    shl eax, 2                      ; qIndex * 4
    add eax, ebx                    ; + optIndex
    mov eax, qOptTable[eax*4]
    ret


Gqoinvalid:
    mov eax,offset emptyStr
    ret
GetQuizOption EndP

CheckQuizAnswer Proc C, qindex:Dword, choice:DWORD
    mov eax, qIndex
    cmp eax,totalQ
    jae cqaWrong

    lea edx,ansTable
    movzx eax,BYTE PTR [edx + eax]

    cmp eax, choice
    jne cqaWrong

    add quizScore,10               ; +10 points
    mov eax,1
    ret
cqawrong:
    xor eax, eax
    ret
CheckQuizAnswer ENDP

END








    

















