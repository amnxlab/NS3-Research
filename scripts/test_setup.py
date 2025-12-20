#!/usr/bin/env python3
"""
Test script to verify the UWSN simulation environment is properly configured.
"""

import sys
from pathlib import Path

def test_python_packages():
    """Test if required Python packages are installed."""
    print("Testing Python packages...")
    required = ['pandas', 'matplotlib', 'seaborn', 'scipy']
    missing = []
    
    for package in required:
        try:
            __import__(package)
            print(f"  ✓ {package}")
        except ImportError:
            print(f"  ✗ {package} (missing)")
            missing.append(package)
    
    if missing:
        print(f"\nInstall missing packages:")
        print(f"  python3 -m pip install {' '.join(missing)}")
        return False
    return True


def test_paths():
    """Test if paths are correctly detected."""
    print("\nTesting path detection...")
    
    # Import the path detection from run_experiments
    sys.path.insert(0, str(Path(__file__).parent))
    from run_experiments import RESEARCH_DIR, NS3_PATH, SRC_DIR, RESULTS_DIR
    
    print(f"  Research Dir: {RESEARCH_DIR}")
    print(f"  NS-3 Path: {NS3_PATH}")
    print(f"  Source Dir: {SRC_DIR}")
    print(f"  Results Dir: {RESULTS_DIR}")
    
    # Check if critical paths exist
    checks = [
        (SRC_DIR / "uwsn-research.cc", "Source file"),
        (NS3_PATH / "ns3", "ns-3 executable"),
        (NS3_PATH / "scratch", "ns-3 scratch folder"),
    ]
    
    all_ok = True
    print("\nVerifying paths...")
    for path, name in checks:
        if path.exists():
            print(f"  ✓ {name}: {path}")
        else:
            print(f"  ✗ {name}: {path} (not found)")
            all_ok = False
    
    return all_ok


def test_ns3_build():
    """Test if ns-3 can be built."""
    print("\nTesting ns-3 build system...")
    
    sys.path.insert(0, str(Path(__file__).parent))
    from run_experiments import NS3_PATH
    
    import subprocess
    
    try:
        result = subprocess.run(
            ["./ns3", "--version"],
            cwd=NS3_PATH,
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode == 0:
            print(f"  ✓ ns-3 version: {result.stdout.strip()}")
            return True
        else:
            print(f"  ✗ ns-3 command failed")
            return False
    except Exception as e:
        print(f"  ✗ Error running ns-3: {e}")
        return False


def main():
    """Run all tests."""
    print("=" * 60)
    print("UWSN Simulation Environment Test")
    print("=" * 60)
    
    tests = [
        ("Python Packages", test_python_packages),
        ("Path Detection", test_paths),
        ("ns-3 Build System", test_ns3_build),
    ]
    
    results = []
    for name, test_func in tests:
        try:
            result = test_func()
            results.append((name, result))
        except Exception as e:
            print(f"\n✗ {name} failed with error: {e}")
            results.append((name, False))
        print()
    
    # Summary
    print("=" * 60)
    print("Test Summary")
    print("=" * 60)
    
    all_passed = True
    for name, passed in results:
        status = "✓ PASS" if passed else "✗ FAIL"
        print(f"  {status}: {name}")
        if not passed:
            all_passed = False
    
    print()
    if all_passed:
        print("🎉 All tests passed! You're ready to run experiments.")
        print("\nNext steps:")
        print("  cd ~/NS3-Research/scripts")
        print("  python3 run_experiments.py --protocols VBF --nodes 100 --runs 1 --simtime 50")
        return 0
    else:
        print("⚠️  Some tests failed. Please fix the issues above.")
        print("\nSee INSTALL.md for troubleshooting help.")
        return 1


if __name__ == "__main__":
    sys.exit(main())

