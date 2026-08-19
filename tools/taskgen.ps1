[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$Api,

    [Parameter(Mandatory = $true)]
    [string]$Definitions,

    [Parameter(Mandatory = $true)]
    [string]$Manager,

    [string]$MainPath,

    [string]$ApiInclude,

    [string]$ManagerInclude,

    [switch]$Check
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Fail {
    param([string]$Message)
    throw "taskgen: $Message"
}

function Add-Line {
    param(
        [System.Text.StringBuilder]$Builder,
        [string]$Line = ''
    )
    [void]$Builder.AppendLine($Line)
}

function Normalize-Content {
    param([string]$Text)
    $crlf = [string][char]13 + [string][char]10
    $lf = [string][char]10
    return $Text.Replace($crlf, $lf).Replace(([string][char]13), $lf)
}

$inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
if (-not (Test-Path -LiteralPath $inputFullPath -PathType Leaf)) {
    Fail "input file not found: $InputPath"
}
$inputDirectory = Split-Path -Parent $inputFullPath
$sourceName = Split-Path -Leaf $inputFullPath

$namespace = $null
$mainFromDsl = $null
$definitionsHeader = $null
$tasks = @()
$autostarts = @()
$caseLists = [System.Collections.Generic.Dictionary[string, object]]::new(
    [System.StringComparer]::Ordinal
)
$currentTask = $null
$currentTaskLine = 0

$lines = [System.IO.File]::ReadAllLines($inputFullPath)
for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
    $lineNumber = $lineIndex + 1
    $line = ($lines[$lineIndex] -split '#', 2)[0].Trim()
    if ([string]::IsNullOrWhiteSpace($line)) {
        continue
    }

    if ($null -ne $currentTask) {
        if ($line -ceq '}') {
            $currentTask = $null
        }
        elseif ($line -cmatch '^case\s+([A-Za-z_][A-Za-z0-9_]*)$') {
            $caseName = $Matches[1]
            if ($caseLists[$currentTask] -ccontains $caseName) {
                Fail ('{0}:{1}: duplicate case {2} for task {3}' -f $InputPath, $lineNumber, $caseName, $currentTask)
            }
            [void]$caseLists[$currentTask].Add($caseName)
        }
        else {
            Fail ('{0}:{1}: unexpected statement {2} in task {3}' -f $InputPath, $lineNumber, $line, $currentTask)
        }
    }
    elseif ($line -cmatch '^namespace\s+(.+)$') {
        if ($null -ne $namespace) {
            Fail ('{0}:{1}: namespace may be declared only once' -f $InputPath, $lineNumber)
        }
        $namespace = $Matches[1].Trim()
        if ($namespace -cnotmatch '^(::)?[A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_][A-Za-z0-9_]*)*$') {
            Fail ('{0}:{1}: invalid namespace {2}' -f $InputPath, $lineNumber, $namespace)
        }
    }
    elseif ($line -cmatch '^main\s+"([^"]+)"$') {
        if ($null -ne $mainFromDsl) {
            Fail ('{0}:{1}: main may be declared only once' -f $InputPath, $lineNumber)
        }
        $mainFromDsl = $Matches[1]
    }
    elseif ($line -cmatch '^definitions\s+"([^"]+)"$') {
        if ($null -ne $definitionsHeader) {
            Fail ('{0}:{1}: definitions may be declared only once' -f $InputPath, $lineNumber)
        }
        $definitionsHeader = $Matches[1]
    }
    elseif ($line -cmatch '^task\s+([A-Za-z_][A-Za-z0-9_]*)(\s+with\s+autostart)?\s*\{$') {
        $taskName = $Matches[1]
        if ($tasks -ccontains $taskName) {
            Fail ('{0}:{1}: duplicate task {2}' -f $InputPath, $lineNumber, $taskName)
        }
        $tasks += $taskName
        $autostarts += (-not [string]::IsNullOrEmpty($Matches[2]))
        $caseLists[$taskName] = [System.Collections.Generic.List[string]]::new()
        $currentTask = $taskName
        $currentTaskLine = $lineNumber
    }
    else {
        Fail ('{0}:{1}: unexpected statement {2}' -f $InputPath, $lineNumber, $line)
    }
}

if ($null -ne $currentTask) {
    Fail ('{0}:{1}: unclosed task {2}' -f $InputPath, $currentTaskLine, $currentTask)
}
if ([string]::IsNullOrEmpty($namespace)) {
    Fail ('{0}: namespace is required' -f $InputPath)
}
if ([string]::IsNullOrEmpty($definitionsHeader)) {
    Fail ('{0}: definitions header is required' -f $InputPath)
}
foreach ($task in $tasks) {
    if ($caseLists[$task].Count -eq 0) {
        Fail ('{0}: task {1} must declare at least one case' -f $InputPath, $task)
    }
    if ($caseLists[$task].Count -gt 256) {
        Fail ('{0}: task {1} exceeds the 256-case limit' -f $InputPath, $task)
    }
}

$selectedMain = $MainPath
if ([string]::IsNullOrEmpty($selectedMain)) {
    $selectedMain = $mainFromDsl
}
if ([string]::IsNullOrEmpty($selectedMain)) {
    $candidates = @()
    $cFile = Join-Path $inputDirectory 'main.c'
    $cppFile = Join-Path $inputDirectory 'main.cpp'
    if (Test-Path -LiteralPath $cFile -PathType Leaf) {
        $candidates += $cFile
    }
    if (Test-Path -LiteralPath $cppFile -PathType Leaf) {
        $candidates += $cppFile
    }
    if ($candidates.Count -eq 0) {
        Fail ('{0}: main file was not found; use -MainPath or a main declaration' -f $InputPath)
    }
    if ($candidates.Count -ne 1) {
        Fail ('{0}: both main.c and main.cpp exist; use -MainPath or a main declaration' -f $InputPath)
    }
    $selectedMain = $candidates[0]
}
elseif (-not [System.IO.Path]::IsPathRooted($selectedMain)) {
    $selectedMain = Join-Path $inputDirectory $selectedMain
}

$selectedMain = [System.IO.Path]::GetFullPath($selectedMain)
if (-not (Test-Path -LiteralPath $selectedMain -PathType Leaf)) {
    Fail "main file not found: $selectedMain"
}

$mainName = Split-Path -Leaf $selectedMain
$extension = [System.IO.Path]::GetExtension($mainName).ToLowerInvariant()
switch ($extension) {
    '.c' { $mode = 'c' }
    '.cc' { $mode = 'cpp' }
    '.cpp' { $mode = 'cpp' }
    '.cxx' { $mode = 'cpp' }
    default { Fail "unsupported main extension in '$mainName'; expected .c or .cpp" }
}

if ([string]::IsNullOrEmpty($ApiInclude)) {
    $ApiInclude = Split-Path -Leaf $Api
}
if ([string]::IsNullOrEmpty($ManagerInclude)) {
    $ManagerInclude = Split-Path -Leaf $Manager
}
$cppNamespace = $namespace.TrimStart([char[]]':')
$apiFullPath = [System.IO.Path]::GetFullPath($Api)
$managerFullPath = [System.IO.Path]::GetFullPath($Manager)
$definitionsFullPath = [System.IO.Path]::GetFullPath($Definitions)

function Type-Stem {
    param([string]$Task)
    return $Task.Substring(0, 1).ToUpperInvariant() + $Task.Substring(1)
}

function Render-Api {
    $builder = [System.Text.StringBuilder]::new()
    Add-Line $builder '// Generated by tools/taskgen.sh or tools/taskgen.ps1; do not edit.'
    Add-Line $builder ("// Source: {0}" -f $sourceName)
    Add-Line $builder ("// Main: {0} ({1})" -f $mainName, $mode)
    Add-Line $builder '#pragma once'
    Add-Line $builder
    Add-Line $builder '#include <cstdint>'
    Add-Line $builder '#include "tasks/Task.hpp"'
    Add-Line $builder ('#include "{0}"' -f $definitionsHeader)
    foreach ($task in $tasks) {
        Add-Line $builder
        Add-Line $builder ("void Loop_{0}(void* rawCtx_, ::uslice::Task* self);" -f $task)
        Add-Line $builder ("void Stop_{0}(void* rawCtx_, ::uslice::Task* self);" -f $task)
    }
    Add-Line $builder
    Add-Line $builder ("namespace {0} {{" -f $cppNamespace)
    Add-Line $builder
    foreach ($task in $tasks) {
        $stem = Type-Stem $task
        Add-Line $builder ("enum class {0}Case : std::uint8_t {{" -f $stem)
        foreach ($caseName in $caseLists[$task]) {
            Add-Line $builder ("    {0}," -f $caseName)
        }
        Add-Line $builder '};'
        Add-Line $builder
        Add-Line $builder ("// Typed view over ::uslice::Task for the {0} case enum; replaces" -f $stem)
        Add-Line $builder ("// static_cast<{0}Case>(self->currentCase()) and" -f $stem)
        Add-Line $builder '// self->gotoCase(static_cast<::uslice::Task::case_t>(...)) with'
        Add-Line $builder '// type-checked calls, so a wrong-task case value cannot compile.'
        Add-Line $builder ("class {0}Handle {{" -f $stem)
        Add-Line $builder '    ::uslice::Task* task_;'
        Add-Line $builder
        Add-Line $builder 'public:'
        Add-Line $builder ("    constexpr explicit {0}Handle(::uslice::Task* task) noexcept" -f $stem)
        Add-Line $builder '        : task_(task) {}'
        Add-Line $builder
        Add-Line $builder ("    [[nodiscard]] constexpr {0}Case currentCase() const noexcept {{" -f $stem)
        Add-Line $builder ("        return static_cast<{0}Case>(task_->currentCase());" -f $stem)
        Add-Line $builder '    }'
        Add-Line $builder ("    constexpr void gotoCase({0}Case value) const noexcept {{" -f $stem)
        Add-Line $builder '        task_->gotoCase(static_cast<::uslice::Task::case_t>(value));'
        Add-Line $builder '    }'
        Add-Line $builder '    constexpr void stop() const noexcept { task_->stop(); }'
        Add-Line $builder '    constexpr bool start() const noexcept { return task_->start(); }'
        Add-Line $builder '    constexpr void raiseFault() const noexcept { task_->raiseFault(); }'
        Add-Line $builder '    [[nodiscard]] constexpr bool isFaulted() const noexcept {'
        Add-Line $builder '        return task_->isFaulted();'
        Add-Line $builder '    }'
        Add-Line $builder '    [[nodiscard]] constexpr bool isRunning() const noexcept {'
        Add-Line $builder '        return task_->isRunning();'
        Add-Line $builder '    }'
        Add-Line $builder '    [[nodiscard]] constexpr bool isStopped() const noexcept {'
        Add-Line $builder '        return task_->isStopped();'
        Add-Line $builder '    }'
        Add-Line $builder '    [[nodiscard]] constexpr ::uslice::TaskState state() const noexcept {'
        Add-Line $builder '        return task_->state();'
        Add-Line $builder '    }'
        Add-Line $builder '};'
        Add-Line $builder
        Add-Line $builder ("{0}Context& {1}Context() noexcept;" -f $stem, $task)
        Add-Line $builder ("::uslice::Task& {0}() noexcept;" -f $task)
        Add-Line $builder
    }
    Add-Line $builder ("}} // namespace {0}" -f $cppNamespace)
    return Normalize-Content $builder.ToString()
}

function Render-Manager {
    $builder = [System.Text.StringBuilder]::new()
    if ($mode -eq 'c') {
        Add-Line $builder '/* Generated by tools/taskgen.sh or tools/taskgen.ps1; do not edit. */'
        Add-Line $builder ("/* Source: {0} */" -f $sourceName)
        Add-Line $builder ("/* Main: {0} (C) */" -f $mainName)
        Add-Line $builder '#pragma once'
        Add-Line $builder
        Add-Line $builder '#ifdef __cplusplus'
        Add-Line $builder 'extern "C" {'
        Add-Line $builder '#endif'
        Add-Line $builder
        Add-Line $builder 'void usliceTaskManager(void);'
        Add-Line $builder 'void usliceTaskManagerPass(void);'
        Add-Line $builder
        Add-Line $builder '#ifdef __cplusplus'
        Add-Line $builder '}'
        Add-Line $builder '#endif'
    }
    else {
        Add-Line $builder '// Generated by tools/taskgen.sh or tools/taskgen.ps1; do not edit.'
        Add-Line $builder ("// Source: {0}" -f $sourceName)
        Add-Line $builder ("// Main: {0} (C++)" -f $mainName)
        Add-Line $builder '#pragma once'
        Add-Line $builder
        Add-Line $builder 'void usliceTaskManager(void);'
        Add-Line $builder 'void usliceTaskManagerPass(void);'
    }
    return Normalize-Content $builder.ToString()
}

function Render-Definitions {
    $builder = [System.Text.StringBuilder]::new()
    Add-Line $builder '// Generated by tools/taskgen.sh or tools/taskgen.ps1; compile this translation unit.'
    Add-Line $builder ("// Source: {0}" -f $sourceName)
    Add-Line $builder ("// Main: {0} ({1})" -f $mainName, $mode)
    Add-Line $builder ('#include "{0}"' -f $ApiInclude)
    Add-Line $builder ('#include "{0}"' -f $ManagerInclude)
    Add-Line $builder '#include "tasks/Task.hpp"'
    Add-Line $builder ('#include "{0}"' -f $definitionsHeader)
    Add-Line $builder

    Add-Line $builder ("namespace {0}::generated_detail {{" -f $cppNamespace)
    Add-Line $builder
    foreach ($task in $tasks) {
        Add-Line $builder ("constexpr ::uslice::Task::Program {0}Program{{" -f $task)
        Add-Line $builder ("    .loop = ::Loop_{0}," -f $task)
        Add-Line $builder ("    .stop = ::Stop_{0}," -f $task)
        Add-Line $builder ("    .caseCount = {0}," -f $caseLists[$task].Count)
        Add-Line $builder '};'
        Add-Line $builder
    }
    Add-Line $builder ("}} // namespace {0}::generated_detail" -f $cppNamespace)
    Add-Line $builder

    Add-Line $builder 'namespace {'
    Add-Line $builder
    Add-Line $builder 'template <typename Context>'
    Add-Line $builder 'class ContextStorage {'
    Add-Line $builder '    mutable Context value{};'
    Add-Line $builder
    Add-Line $builder 'public:'
    Add-Line $builder '    constexpr Context& get() const noexcept { return value; }'
    Add-Line $builder '};'
    Add-Line $builder
    Add-Line $builder 'template <const ::uslice::Task::Program* ProgramPtr>'
    Add-Line $builder 'class TaskStorage {'
    Add-Line $builder '    mutable ::uslice::Task value;'
    Add-Line $builder
    Add-Line $builder 'public:'
    Add-Line $builder '    consteval explicit TaskStorage('
    Add-Line $builder '        ::uslice::Task::Definition<ProgramPtr> definition) noexcept'
    Add-Line $builder '        : value(definition) {}'
    Add-Line $builder '    constexpr ::uslice::Task& get() const noexcept { return value; }'
    Add-Line $builder '};'
    Add-Line $builder
    for ($taskIndex = 0; $taskIndex -lt $tasks.Count; $taskIndex++) {
        $task = $tasks[$taskIndex]
        $stem = Type-Stem $task
        Add-Line $builder ("constinit const ContextStorage<{0}Context> {1}ContextStorage{{}};" -f $stem, $task)
        Add-Line $builder ("constinit const TaskStorage<&{0}::generated_detail::{1}Program> {1}Storage{{" -f $cppNamespace, $task)
        Add-Line $builder ("    ::uslice::Task::Definition<&{0}::generated_detail::{1}Program>{{" -f $cppNamespace, $task)
        Add-Line $builder ("    .context = &{0}ContextStorage.get()," -f $task)
        Add-Line $builder ("    .autostart = {0}," -f ($(if ($autostarts[$taskIndex]) { 'true' } else { 'false' })))
        Add-Line $builder '    }'
        Add-Line $builder '};'
        Add-Line $builder
    }
    Add-Line $builder '} // namespace'
    Add-Line $builder
    Add-Line $builder ("namespace {0} {{" -f $cppNamespace)
    Add-Line $builder
    foreach ($task in $tasks) {
        $stem = Type-Stem $task
        Add-Line $builder ("{0}Context& {1}Context() noexcept {{ return {1}ContextStorage.get(); }}" -f $stem, $task)
        Add-Line $builder ("::uslice::Task& {0}() noexcept {{ return {0}Storage.get(); }}" -f $task)
        Add-Line $builder
    }
    Add-Line $builder ("}} // namespace {0}" -f $cppNamespace)
    Add-Line $builder
    Add-Line $builder ("namespace {0}::generated_detail {{" -f $cppNamespace)
    Add-Line $builder
    Add-Line $builder '// Nodes are declared tail-first; traversal remains in DSL order.'
    if ($tasks.Count -eq 0) {
        Add-Line $builder '// Scheduler order: (empty)'
    }
    else {
        Add-Line $builder ("// Scheduler order: {0}" -f ($tasks -join ' -> '))
    }

    $nextLink = 'nullptr'
    for ($taskIndex = $tasks.Count - 1; $taskIndex -ge 0; $taskIndex--) {
        $task = $tasks[$taskIndex]
        $linkName = "taskRegistry_{0}Link" -f $task
        Add-Line $builder ("constexpr ::uslice::TaskLink {0}{{&{1}Storage.get(), {2}}};" -f $linkName, $task, $nextLink)
        $nextLink = '&' + $linkName
    }
    Add-Line $builder
    Add-Line $builder ("}} // namespace {0}::generated_detail" -f $cppNamespace)
    Add-Line $builder

    if ($nextLink -eq 'nullptr') {
        $registryHead = 'nullptr'
    }
    else {
        $registryHead = '&{0}::generated_detail::{1}' -f $cppNamespace, $nextLink.Substring(1)
    }
    Add-Line $builder 'namespace {'
    Add-Line $builder ("constinit const ::uslice::TaskRegistry taskRegistry{{{0}}};" -f $registryHead)
    Add-Line $builder '}'
    Add-Line $builder

    if ($mode -eq 'c') {
        Add-Line $builder 'extern "C" void usliceTaskManagerPass(void) {'
    }
    else {
        Add-Line $builder 'void usliceTaskManagerPass(void) {'
    }
    Add-Line $builder '    taskRegistry.executePass();'
    Add-Line $builder '}'
    Add-Line $builder

    if ($mode -eq 'c') {
        Add-Line $builder 'extern "C" void usliceTaskManager(void) {'
    }
    else {
        Add-Line $builder 'void usliceTaskManager(void) {'
    }
    Add-Line $builder '    while (true) {'
    Add-Line $builder '        taskRegistry.executePass();'
    Add-Line $builder '    }'
    Add-Line $builder '}'
    return Normalize-Content $builder.ToString()
}

function Update-GeneratedFile {
    param(
        [string]$Path,
        [string]$Content
    )
    $directory = Split-Path -Parent $Path
    if (-not [string]::IsNullOrEmpty($directory)) {
        [void][System.IO.Directory]::CreateDirectory($directory)
    }
    $oldContent = $null
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        $oldContent = Normalize-Content ([System.IO.File]::ReadAllText($Path))
    }
    if ($oldContent -ceq $Content) {
        return $true
    }
    if ($Check) {
        Write-Error "outdated generated file: $Path"
        return $false
    }
    $temporary = [System.IO.Path]::GetTempFileName()
    try {
        $utf8 = [System.Text.UTF8Encoding]::new($false)
        [System.IO.File]::WriteAllText($temporary, $Content, $utf8)
        Move-Item -LiteralPath $temporary -Destination $Path -Force
    }
    finally {
        if (Test-Path -LiteralPath $temporary) {
            Remove-Item -LiteralPath $temporary -Force
        }
    }
    return $true
}

$valid = $true
$valid = (Update-GeneratedFile $apiFullPath (Render-Api)) -and $valid
$valid = (Update-GeneratedFile $managerFullPath (Render-Manager)) -and $valid
$valid = (Update-GeneratedFile $definitionsFullPath (Render-Definitions)) -and $valid
if (-not $valid) {
    exit 1
}
