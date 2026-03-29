package tests

import (
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"syscall"
	"time"
)

const (
	defaultMachine = "P340"
	defaultEmuPath = "/Applications/zesarux.app/Contents/MacOS/zesarux"
	defaultHost    = "127.0.0.1"
	defaultPort    = 10000
)

func resolvePort() int {
	if env := os.Getenv("ZRCP_PORT"); env != "" {
		if port, err := strconv.Atoi(env); err == nil && port > 0 && port < 65536 {
			return port
		}
	}

	return defaultPort
}

func resolveEmulator() (string, error) {
	if env := os.Getenv("ZESARUX_BIN"); env != "" {
		if info, err := os.Stat(env); err == nil && info.Mode().Perm()&0o111 != 0 {
			return env, nil
		}
	}

	if info, err := os.Stat(defaultEmuPath); err == nil && info.Mode().Perm()&0o111 != 0 {
		return defaultEmuPath, nil
	}

	p, err := exec.LookPath("zesarux")
	if err != nil {
		return "", fmt.Errorf("emulator not found (set ZESARUX_BIN or install zesarux)")
	}
	return p, nil
}

func waitForPort(host string, port int, timeout time.Duration) bool {
	deadline := time.Now().Add(timeout)
	addr := fmt.Sprintf("%s:%d", host, port)
	for time.Now().Before(deadline) {
		conn, err := net.DialTimeout("tcp", addr, time.Second)
		if err == nil {
			_ = conn.Close()
			return true
		}
		time.Sleep(100 * time.Millisecond)
	}
	return false
}

func zrcpCommand(host string, port int, cmd string) (string, error) {
	addr := fmt.Sprintf("%s:%d", host, port)
	conn, err := net.DialTimeout("tcp", addr, 2*time.Second)
	if err != nil {
		return "", err
	}
	defer conn.Close()

	_ = conn.SetDeadline(time.Now().Add(2 * time.Second))

	if _, err := io.WriteString(conn, cmd+"\n"); err != nil {
		return "", err
	}

	var out []byte
	buf := make([]byte, 4096)
	for {
		n, readErr := conn.Read(buf)
		if n > 0 {
			out = append(out, buf[:n]...)
		}
		if readErr == nil {
			continue
		}
		if errors.Is(readErr, io.EOF) {
			break
		}
		if ne, ok := readErr.(net.Error); ok && ne.Timeout() {
			break
		}
		return "", readErr
	}

	return string(out), nil
}

func spawnEmulator(emulatorPath string, port int) (*exec.Cmd, error) {
	args := []string{
		"--machine", defaultMachine,
		"--emulatorspeed", "100",
		"--fastautoload",
		"--enable-remoteprotocol",
		"--remoteprotocol-port", fmt.Sprintf("%d", port),
		"--noconfigfile",
		"--vo", "null",
		"--ao", "null",
	}

	cmd := exec.Command(emulatorPath, args...)

	// Build a clean env with HOME replaced (not duplicated) so ZEsarUX
	// cannot read or write the user's real ~/.zesaruxrc.
	const safeHome = "/tmp/zesarux-smoketest-home-go"
	baseEnv := os.Environ()
	filteredEnv := make([]string, 0, len(baseEnv)+1)
	for _, e := range baseEnv {
		if !strings.HasPrefix(e, "HOME=") {
			filteredEnv = append(filteredEnv, e)
		}
	}
	filteredEnv = append(filteredEnv, "HOME="+safeHome)
	cmd.Env = filteredEnv

	if err := os.MkdirAll(safeHome, 0o755); err != nil {
		return nil, err
	}

	if err := cmd.Start(); err != nil {
		return nil, err
	}
	return cmd, nil
}

func stopEmulator(cmd *exec.Cmd, host string, port int) {
	if cmd == nil || cmd.Process == nil {
		return
	}

	_, _ = zrcpCommand(host, port, "exit-emulator")

	waitDone := make(chan error, 1)
	go func() {
		waitDone <- cmd.Wait()
	}()

	select {
	case <-time.After(5 * time.Second):
		_ = cmd.Process.Signal(syscall.SIGTERM)
		select {
		case <-time.After(2 * time.Second):
			_ = cmd.Process.Kill()
			<-waitDone
		case <-waitDone:
		}
	case <-waitDone:
	}
}

func absPathFromRepo(repoRoot string, rel string) (string, error) {
	if filepath.IsAbs(rel) {
		return rel, nil
	}
	path := filepath.Join(repoRoot, rel)
	return filepath.Abs(path)
}

func containsAll(s string, needles ...string) bool {
	for _, n := range needles {
		if !strings.Contains(s, n) {
			return false
		}
	}
	return true
}

var errTimeout = errors.New("timeout")
