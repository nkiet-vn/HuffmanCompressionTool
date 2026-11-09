param(
    [string] $BuildDir = "build-ucrt64",
    [string] $DistributionDir = "dist"
)
$ErrorActionPreference = "Stop"
$project = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$build = Join-Path $project $BuildDir
$dist = Join-Path $project $DistributionDir
$stage = Join-Path $dist "TheTypeFoundry-portable"
New-Item -ItemType Directory -Force $stage | Out-Null
cmake --build $build --config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
$gui = Get-ChildItem $build -Recurse -Filter huffman_gui.exe | Select-Object -First 1 -ExpandProperty FullName
$cli = Get-ChildItem $build -Recurse -Filter huffman.exe | Select-Object -First 1 -ExpandProperty FullName
if (-not $gui -or -not $cli) { throw "Missing GUI or CLI executable" }
Copy-Item $gui $stage
Copy-Item $cli $stage
$deployer = Get-Command windeployqt6, windeployqt -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $deployer) { throw "windeployqt not found in PATH" }
& $deployer.Source --release --qmldir (Join-Path $project "gui\qml") (Join-Path $stage "huffman_gui.exe")
if ($LASTEXITCODE -ne 0) { throw "Qt deploy failed" }
$zip = Join-Path $dist "TypeFoundry-1.0.0-portable.zip"
if(Test-Path $zip){Remove-Item $zip}
Compress-Archive -Path "$stage\*" -DestinationPath $zip
Write-Host "Portable archive: $zip"
$installer = Get-Command makensis -ErrorAction SilentlyContinue
if ($installer) {
    $setup = Join-Path $dist "TypeFoundry-1.0.0-Setup.exe"
    & $installer.Source "/DSTAGE=$stage" "/DOUTPUT=$setup" (Join-Path $project "gui\packaging\installer.nsi")
    if ($LASTEXITCODE -ne 0) { throw "NSIS build failed" }
    Write-Host "Installer: $setup"
} else {
    Write-Warning "NSIS (makensis) not available. Portable ZIP created; installer skipped."
}
