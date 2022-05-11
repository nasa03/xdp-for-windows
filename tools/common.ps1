#
# Helper functions for XDP project.
#

function Get-CurrentBranch {
    $env:GIT_REDIRECT_STDERR = '2>&1'
    $CurrentBranch = $null
    try {
        $CurrentBranch = git branch --show-current
        if ([string]::IsNullOrWhiteSpace($CurrentBranch)) {
            throw
        }
    } catch {
        Write-Error "Failed to get branch from git"
    }
    return $CurrentBranch
}

# Returns the target or current git branch.
function Get-BuildBranch {
    $PrBranchName = $env:SYSTEM_PULLREQUEST_TARGETBRANCH
    if ([string]::IsNullOrWhiteSpace($PrBranchName)) {
        # Mainline build, just get branch name
        $AzpBranchName = $env:BUILD_SOURCEBRANCH
        if ([string]::IsNullOrWhiteSpace($AzpBranchName)) {
            # Non-Azure build
            $BranchName = Get-CurrentBranch
        } else {
            # Azure Build
            $BuildReason = $env:BUILD_REASON
            if ("Manual" -eq $BuildReason) {
                $BranchName = "unknown"
            } else {
                $AzpBranchName -match 'refs/heads/(.+)' | Out-Null
                $BranchName = $Matches[1]
            }
        }
    } else {
        # PR Build
        $BranchName = $PrBranchName
    }
    return $BranchName
}

function Get-VsTestPath {
    # Unfortunately CI doesn't add vstest to PATH. Test existence of vstest
    # install paths instead.

    $ManualVsTestPath = "$(Split-Path $PSScriptRoot -Parent)\artifacts\Microsoft.TestPlatform\tools\net451\Common7\IDE\Extensions\TestPlatform"
    if (Test-Path $ManualVsTestPath) {
        return $ManualVsTestPath
    }

    $CiVsTestPath = "${Env:ProgramFiles(X86)}\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\Extensions\TestPlatform"
    if (Test-Path $CiVsTestPath) {
        return $CiVsTestPath
    }

    return $null
}
