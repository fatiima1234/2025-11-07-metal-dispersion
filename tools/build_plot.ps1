param(
    [string]$file
)

$projectDir    = Split-Path -Path (Resolve-Path $file)
$pythonInclude = "C:\Users\ffmir\AppData\Local\Programs\Python\Python313\include"
$numpyInclude  = "C:\Users\ffmir\AppData\Local\Programs\Python\Python313\Lib\site-packages\numpy\_core\include"
$pythonLib     = "C:\Users\ffmir\AppData\Local\Programs\Python\Python313\libs"

$out = [System.IO.Path]::ChangeExtension($file, ".exe")

Write-Host "Compiling $file ..."
$compileOutput = g++ $file `
    -I"$projectDir" `
    -I"$pythonInclude" `
    -I"$numpyInclude" `
    -L"$pythonLib" `
    -lpython313 -std=c++17 -o $out

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed. Check errors above." -ForegroundColor Red
    Write-Host $compileOutput -ForegroundColor Yellow
    exit 1
}

Write-Host "Running $out ..."
& $out