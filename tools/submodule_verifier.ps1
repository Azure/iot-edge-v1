function Verify-Submodules()
{
    $ret = 0;
    
    #Set array of submodules to skip if arguments were supplied
    if($args){
        $skips = $args.Split(" ")
    }
    
    #Get list of submodules in repo
    $submodules_string = (iex "git config -f .gitmodules --list").Replace("submodule.","")

    #For each submodule compare the head to the remote head
    Get-ChildItem -hidden -Path "$pwd\.git\modules\*" | % {
        if($skips -notcontains $_.name){
            $hash = Get-Content $_\HEAD
            $submodule_name = $_.name
            
            $url = $submodules_string | ? { $_ -match "$submodule_name.url"}
            $url = $url.Substring($url.IndexOf("=")+1)
            
            #Grab hash of the latest develop commit if there is a develop branch, otherwise the HEAD.
            $remote_head_hash = ((iex "git ls-remote $url refs/heads/develop") -split '\s+')[0]
            if(!($remote_head_hash)){
                $remote_head_hash = ((iex "git ls-remote $url HEAD") -split '\s+')[0]
            }
            
            if($hash -ne $remote_head_hash){
                Write-Host "Submodule $submodule_name is not up to date.`nLocal head [$hash] does not match remote head [$remote_head_hash]" -f red
                $ret = 1;
            }
        }
    }
    exit $ret;
}

Verify-Submodules $args