#!/usr/bin/env bash
ANDROID_HOME="c:\tools\android"

zips=(
  "openjdk-19.0.1_windows-x64_bin.zip" = @('c:\tools', 'jdk-19.0.1');
  "platform-tools_r34.0.0-windows.zip" = @($env:ANDROID_HOME, "platform-tools");
  "commandlinetools-win-9123335_latest.zip" = @("$env:ANDROID_HOME","cmdline-tools");
  "gradle-8.0.2-bin.zip" = @("c:\tools", "gradle-8.0.2", 'bin')
)

(New-Item -ItemType Directory -Force -Path $env:ANDROID_HOME) > $null

foreach ($key in $zips.keys) {
  echo "key: $($key): $($zips[$key])"
  $dest = Join-String('\', $zips[$key][0], $zips[$key][1])
  echo "Test path: $($dest)"
  if ((Test-Path $dest -PathType Container) -eq $false) {
    echo "Expand-Archive -Path $key -DestinationPath $($zips[$key][0])"
    Expand-Archive -Path $key -DestinationPath $zips[$key][0]

    if ($zips[$key][1] -eq "cmdline-tools") {
      echo "Move-Item `"$dest\*`" -Dest `"$dest\latest\`""

      (New-Item -ItemType Directory -Force -Path "$env:ANDROID_HOME\latest") > $null
      Move-Item "$dest\*" -Dest "$env:ANDROID_HOME\latest\"
      Move-Item "$env:ANDROID_HOME\latest" -Dest "$dest\"
    }
  }

  if ($zips[$key].Count -gt 2) {
    $path = Join-String('\', $zips[$key][0], $zips[$key][1], $zips[$key][2])
    echo "Add path: $path"

    $env:Path="$path;$env:PATH"
    [Environment]::SetEnvironmentVariable("PATH", "$path;$($env:PATH)")
  }

    if ($key -eq "openjdk-19.0.1_windows-x64_bin.zip") {
      echo "Set JAVA_HOME = $dest"
      # Doesn't work
      [Environment]::SetEnvironmentVariable("JAVA_HOME", $dest)
  }
}

echo "JAVA_HOME=$JAVA_HOME"
echo "ANDROID_HOME=$ANDROID_HOME"
