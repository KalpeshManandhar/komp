Param(
    [Parameter(Mandatory, HelpMessage = "Usage: <script> <exec>")]
    [string]$exec_path,
    [string]$test_name

)

$test_folder = ".\tests\parser"

. "$test_folder/errors_info.ps1"
$errors_found = $errors_expected.Clone()

$files = Get-ChildItem -Path $test_folder"\*" -Include "*.c" 
if ($PSBoundParameters.ContainsKey('test_name')){
    $files = $files | Where-Object -Property Name -eq $test_name
}

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