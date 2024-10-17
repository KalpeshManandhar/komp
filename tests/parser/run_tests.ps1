Param(
    [Parameter(Mandatory, HelpMessage = "Usage: <script> <exec> <test_folder>")]
    [string]$exec_path
)

. ".\tests\parser\errors_info.ps1"

$test_folder = ".\tests\parser"
$errors_found = $errors_expected.Clone()

$files = Get-ChildItem -Path $test_folder"\*" -Include "*.c" 
foreach ($file in $files){  
    Write-Host "Test: " $file.Name -ForegroundColor Cyan  
    & "$exec_path" $file   
    $errors_found[$file.Name] = $LASTEXITCODE
} 

foreach ($file in $files){    
    if ($errors_found[$file.Name] -eq $errors_expected[$file.Name]){
        Write-Host -NoNewline "Test passed: " $file.Name " " -ForegroundColor Green    
    }
    elseif ($errors_found[$file.Name] -gt $errors_expected[$file.Name]){
        Write-Host -NoNewline "Test passed: " $file.Name " " -ForegroundColor DarkYellow    
    }
    else {
        Write-Host -NoNewline "Test failed: " $file.Name " " -ForegroundColor Red    
    }
    Write-Host $errors_found[$file.Name]"/"$errors_expected[$file.Name] "errors found."
} 