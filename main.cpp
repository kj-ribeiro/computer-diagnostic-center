// =============================================================
// COMPUTER DIAGNOSTIC CENTER - PROFESSIONAL EDITION
// =============================================================
// Evolução do projeto original:
// - Dashboard profissional
// - Health Score calculado a partir de dados reais
// - Full System Diagnostic
// - Network Diagnostics
// - Hardware / Storage / Security
// - Logs
// - Relatório técnico exportável
// - Ferramentas administrativas do Windows
//
// Requisitos:
// - Windows
// - raylib
// - Compilador C++
// - Sem dependencias adicionais de linker para a recolha de rede.
//
// O programa NÃO cria valores fictícios. Quando uma informação não
// pode ser obtida, apresenta N/A.
// =============================================================

// Compatibilidade Raylib + Windows/MinGW
// Raylib define Rectangle/CloseWindow/ShowCursor; Windows tambem
// possui simbolos com os mesmos nomes. Incluimos Raylib primeiro e
// renomeamos temporariamente os simbolos Windows conflitantes.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
#define NOGDI
#endif

#include "raylib.h"

#define CloseWindow Win32_CloseWindow
#define ShowCursor Win32_ShowCursor
#define DrawTextEx Win32_DrawTextEx
#define LoadImageA Win32_LoadImageA
#define LoadImageW Win32_LoadImageW
#define LoadImage Win32_LoadImage

#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>

#undef CloseWindow
#undef ShowCursor
#undef DrawTextEx
#undef LoadImageA
#undef LoadImageW
#undef LoadImage

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>


// =============================================================
// CONFIGURAÇÃO
// =============================================================

const int LARGURA_JANELA = 1200;
const int ALTURA_JANELA = 820;
const int LARGURA_MENU = 245;

const Color COR_FUNDO          = {6, 14, 16, 255};
const Color COR_CABECALHO      = {10, 32, 36, 255};
const Color COR_TITULO         = {80, 230, 235, 255};
const Color COR_BOTAO          = {14, 48, 52, 255};
const Color COR_BOTAO_HOVER    = {25, 95, 100, 255};
const Color COR_BOTAO_PREMIDO  = {60, 210, 215, 255};
const Color COR_TEXTO          = {190, 235, 238, 255};
const Color COR_TEXTO_DIM      = {125, 180, 185, 255};
const Color COR_CONTORNO       = {55, 180, 185, 200};
const Color COR_PAINEL         = {10, 24, 27, 255};
const Color COR_PAINEL_2       = {12, 30, 33, 255};
const Color COR_VERDE          = {75, 220, 145, 255};
const Color COR_AMARELO        = {240, 190, 70, 255};
const Color COR_VERMELHO       = {235, 85, 90, 255};
const Color COR_CINZENTO       = {100, 125, 130, 255};

// Logo Kj - usado na interface e como ícone da janela.
Texture2D LOGO_KJ = {0};
bool LOGO_KJ_VALIDO = false;

// =============================================================
// MODELOS DE DADOS
// =============================================================

class SystemData
{
public:
	// Os dados são públicos porque a interface gráfica necessita
	// de os consultar diretamente. A lógica de atualização fica
	// encapsulada nas classes de serviço abaixo.
	std::string computer = "N/A";
	std::string user = "N/A";
	std::string architecture = "N/A";
	std::string uptime = "N/A";
	std::string cpuModel = "N/A";
	std::string windowsDirectory = "N/A";
	
	int logicalProcessors = 0;
	int cpuUsage = 0;
	int ramUsage = 0;
	double totalRamGB = 0.0;
	double usedRamGB = 0.0;
	double freeRamGB = 0.0;
	int processCount = 0;
	
	unsigned long long diskTotalGB = 0;
	unsigned long long diskFreeGB = 0;
	unsigned long long diskUsedGB = 0;
	int diskUsage = 0;
	
	std::string adapter = "N/A";
	std::string localIP = "N/A";
	std::string gateway = "N/A";
	std::string dns = "N/A";
	std::string mac = "N/A";
	bool networkAvailable = false;
	bool internetAvailable = false;
	
	bool defenderAvailable = false;
	bool defenderEnabled = false;
	bool firewallAvailable = false;
	bool firewallEnabled = false;
	
	int healthScore = 0;
	std::string healthStatus = "N/A";
};

class DiagnosticResult
{
public:
	std::string name;
	std::string status;
	std::string detail;
	int severity = 0;
	
	DiagnosticResult() = default;
	
	DiagnosticResult(
					 const std::string& nome,
					 const std::string& estado,
					 const std::string& detalhe,
					 int severidade)
	: name(nome),
	status(estado),
	detail(detalhe),
	severity(severidade)
	{
	}
};

class LogEntry
{
public:
	std::string time;
	std::string message;
	
	LogEntry() = default;
	
	LogEntry(
			 const std::string& hora,
			 const std::string& mensagem)
	: time(hora),
	message(mensagem)
	{
	}
};

enum class Page
{
	DASHBOARD,
	DIAGNOSTICS,
	NETWORK,
	HARDWARE,
	STORAGE,
	SECURITY,
	ADMINISTRATION,
	REPORTS
};

// =============================================================
// FUNÇÕES AUXILIARES
// =============================================================

std::string Agora()
{
	std::time_t t = std::time(nullptr);
	std::tm tmLocal{};
	localtime_s(&tmLocal, &t);
	
	char buffer[64];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmLocal);
	return buffer;
}

std::string DataHoraFicheiro()
{
	std::time_t t = std::time(nullptr);
	std::tm tmLocal{};
	localtime_s(&tmLocal, &t);
	
	char buffer[64];
	std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tmLocal);
	return buffer;
}

void AdicionarLog(std::vector<LogEntry>& logs, const std::string& mensagem)
{
	LogEntry e;
	e.time = Agora();
	e.message = mensagem;
	logs.push_back(e);
	
	if (logs.size() > 200)
		logs.erase(logs.begin());
	
	std::ofstream ficheiro("diagnostic.log", std::ios::app);
	if (ficheiro)
		ficheiro << "[" << e.time << "] " << e.message << "\n";
}

bool ExecutarComandoSilencioso(const std::string& comando)
{
	std::string cmd = "cmd.exe /C " + comando + " >nul 2>&1";
	int resultado = std::system(cmd.c_str());
	return resultado == 0;
}

std::string ExecutarComando(const std::string& comando)
{
	std::string resultado;
	FILE* pipe = _popen(comando.c_str(), "r");
	
	if (!pipe)
		return "";
	
	char buffer[512];
	
	while (fgets(buffer, sizeof(buffer), pipe))
		resultado += buffer;
	
	_pclose(pipe);
	return resultado;
}

std::string ObterValorRegisto(const char* subKey, const char* valueName)
{
	HKEY hKey = nullptr;
	
	if (RegOpenKeyExA(
					  HKEY_LOCAL_MACHINE,
					  subKey,
					  0,
					  KEY_READ,
					  &hKey) != ERROR_SUCCESS)
	{
		return "N/A";
	}
	
	char buffer[512];
	DWORD tamanho = sizeof(buffer);
	DWORD tipo = 0;
	
	LONG resultado = RegQueryValueExA(
									  hKey,
									  valueName,
									  nullptr,
									  &tipo,
									  reinterpret_cast<LPBYTE>(buffer),
									  &tamanho);
	
	RegCloseKey(hKey);
	
	if (resultado != ERROR_SUCCESS)
		return "N/A";
	
	buffer[sizeof(buffer) - 1] = '\0';
	return std::string(buffer);
}

Color CorEstado(int severity)
{
	if (severity == 2) return COR_VERMELHO;
	if (severity == 1) return COR_AMARELO;
	return COR_VERDE;
}

std::string TextoEstado(int severity)
{
	if (severity == 2) return "CRITICAL";
	if (severity == 1) return "WARNING";
	return "NORMAL";
}

// =============================================================
// RECOLHA DE INFORMAÇÃO DO SISTEMA
// =============================================================

class SystemMonitor
{
private:
	ULARGE_INTEGER idleAnterior{};
	ULARGE_INTEGER kernelAnterior{};
	ULARGE_INTEGER userAnterior{};
	bool primeiraLeitura = true;
	
public:
	
	void AtualizarCPU(SystemData& d)
	{
		FILETIME idleTime{}, kernelTime{}, userTime{};
		
		if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
			return;
		
		ULARGE_INTEGER idleAtual;
		idleAtual.LowPart = idleTime.dwLowDateTime;
		idleAtual.HighPart = idleTime.dwHighDateTime;
		
		ULARGE_INTEGER kernelAtual;
		kernelAtual.LowPart = kernelTime.dwLowDateTime;
		kernelAtual.HighPart = kernelTime.dwHighDateTime;
		
		ULARGE_INTEGER userAtual;
		userAtual.LowPart = userTime.dwLowDateTime;
		userAtual.HighPart = userTime.dwHighDateTime;
		
		if (!primeiraLeitura)
		{
			unsigned long long idleDelta =
			idleAtual.QuadPart - idleAnterior.QuadPart;
			
			unsigned long long kernelDelta =
			kernelAtual.QuadPart - kernelAnterior.QuadPart;
			
			unsigned long long userDelta =
			userAtual.QuadPart - userAnterior.QuadPart;
			
			unsigned long long total =
			kernelDelta + userDelta;
			
			if (total > 0)
			{
				double uso =
				100.0 *
				static_cast<double>(total - idleDelta) /
				static_cast<double>(total);
				
				d.cpuUsage =
				std::clamp(static_cast<int>(uso), 0, 100);
			}
		}
		
		idleAnterior = idleAtual;
		kernelAnterior = kernelAtual;
		userAnterior = userAtual;
		primeiraLeitura = false;
	}
	
	void AtualizarRAM(SystemData& d)
	{
		MEMORYSTATUSEX mem{};
		mem.dwLength = sizeof(mem);
		
		if (!GlobalMemoryStatusEx(&mem))
			return;
		
		d.ramUsage = static_cast<int>(mem.dwMemoryLoad);
		d.totalRamGB =
		static_cast<double>(mem.ullTotalPhys) /
		(1024.0 * 1024.0 * 1024.0);
		
		d.freeRamGB =
		static_cast<double>(mem.ullAvailPhys) /
		(1024.0 * 1024.0 * 1024.0);
		
		d.usedRamGB = d.totalRamGB - d.freeRamGB;
	}
	
	void AtualizarProcessos(SystemData& d)
	{
		HANDLE snapshot =
		CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		
		if (snapshot == INVALID_HANDLE_VALUE)
			return;
		
		PROCESSENTRY32 entry{};
		entry.dwSize = sizeof(entry);
		
		int count = 0;
		
		if (Process32First(snapshot, &entry))
		{
			do
			{
				++count;
			}
			while (Process32Next(snapshot, &entry));
		}
		
		CloseHandle(snapshot);
		d.processCount = count;
	}
	
	void Atualizar(SystemData& d)
	{
		AtualizarCPU(d);
		AtualizarRAM(d);
		AtualizarProcessos(d);
	}
};

// =============================================================
// CLASSES DE SERVIÇO - PROGRAMAÇÃO ORIENTADA A OBJETOS
// =============================================================

// -------------------------------------------------------------
// SystemCollector
// Responsável por recolher informação real do Windows.
// -------------------------------------------------------------
class SystemCollector
{
public:
	void AtualizarInformacoesSistema(SystemData& d)
	{
		char nome[256];
		DWORD tamanho = sizeof(nome);
		
		if (GetComputerNameA(nome, &tamanho))
			d.computer = nome;
		
		char utilizador[256];
		DWORD tamanhoUtilizador = sizeof(utilizador);
		
		if (GetUserNameA(utilizador, &tamanhoUtilizador))
			d.user = utilizador;
		
		SYSTEM_INFO info{};
		GetNativeSystemInfo(&info);
		
		d.logicalProcessors =
		static_cast<int>(info.dwNumberOfProcessors);
		
		if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			d.architecture = "64 bits (x64)";
		else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
			d.architecture = "32 bits (x86)";
		else
			d.architecture = "N/A";
		
		char windowsDir[MAX_PATH];
		if (GetWindowsDirectoryA(windowsDir, MAX_PATH))
			d.windowsDirectory = windowsDir;
		
		d.cpuModel = ObterValorRegisto(
									   "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
									   "ProcessorNameString");
		
		ULONGLONG minutos = GetTickCount64() / 60000ULL;
		
		unsigned long long dias = minutos / 1440ULL;
		unsigned long long horas = (minutos % 1440ULL) / 60ULL;
		unsigned long long mins = minutos % 60ULL;
		
		std::ostringstream uptime;
		
		if (dias > 0)
			uptime << dias << " days, "
			<< horas << " hours, "
			<< mins << " minutes";
		else
			uptime << horas << " hours, "
			<< mins << " minutes";
		
		d.uptime = uptime.str();
	}
	
	void AtualizarDisco(SystemData& d)
	{
		ULARGE_INTEGER freeBytes{}, totalBytes{}, totalFreeBytes{};
		
		if (!GetDiskFreeSpaceExA(
								 "C:\\",
								 &freeBytes,
								 &totalBytes,
								 &totalFreeBytes))
		{
			d.diskTotalGB = 0;
			d.diskFreeGB = 0;
			d.diskUsedGB = 0;
			d.diskUsage = 0;
			return;
		}
		
		d.diskTotalGB =
		totalBytes.QuadPart /
		(1024ULL * 1024ULL * 1024ULL);
		
		d.diskFreeGB =
		totalFreeBytes.QuadPart /
		(1024ULL * 1024ULL * 1024ULL);
		
		d.diskUsedGB =
		d.diskTotalGB >= d.diskFreeGB
		? d.diskTotalGB - d.diskFreeGB
		: 0;
		
		if (d.diskTotalGB > 0)
		{
			d.diskUsage =
			static_cast<int>(
							 (100.0 *
							  static_cast<double>(d.diskUsedGB)) /
							  static_cast<double>(d.diskTotalGB));
		}
	}
	
	void AtualizarRede(SystemData& d)
	{
		d.networkAvailable = false;
		d.internetAvailable = false;
		d.adapter = "N/A";
		d.localIP = "N/A";
		d.gateway = "N/A";
		d.dns = "N/A";
		d.mac = "N/A";
		
		// Recolha de rede sem GetAdaptersInfo/GetNetworkParams.
		// Isto evita a necessidade de ligar a iphlpapi.lib no MinGW.
		// Usamos WMI/CIM através do PowerShell, com propriedades estáveis.
		const std::string comando =
		"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \""
		"$a=Get-CimInstance Win32_NetworkAdapterConfiguration | "
		"Where-Object {$_.IPEnabled -eq $true} | Select-Object -First 1; "
		"if($a){ "
		"Write-Output $a.Description; "
		"Write-Output ($a.IPAddress | Select-Object -First 1); "
		"Write-Output ($a.DefaultIPGateway | Select-Object -First 1); "
		"Write-Output ($a.DNSServerSearchOrder | Select-Object -First 1); "
		"Write-Output $a.MACAddress "
		"}\"";
		
		std::string saida = ExecutarComando(comando);
		
		std::vector<std::string> linhas;
		std::stringstream ss(saida);
		std::string linha;
		
		while (std::getline(ss, linha))
		{
			while (!linha.empty() &&
				   (linha.back() == '\r' || linha.back() == '\n'))
			{
				linha.pop_back();
			}
			
			if (!linha.empty())
				linhas.push_back(linha);
		}
		
		if (!linhas.empty())
		{
			if (linhas.size() > 0 && !linhas[0].empty())
				d.adapter = linhas[0];
			
			if (linhas.size() > 1 && !linhas[1].empty())
				d.localIP = linhas[1];
			
			if (linhas.size() > 2 && !linhas[2].empty())
				d.gateway = linhas[2];
			
			if (linhas.size() > 3 && !linhas[3].empty())
				d.dns = linhas[3];
			
			if (linhas.size() > 4 && !linhas[4].empty())
				d.mac = linhas[4];
			
			if (d.localIP != "N/A" &&
				d.localIP != "0.0.0.0" &&
				d.localIP != "127.0.0.1")
			{
				d.networkAvailable = true;
			}
		}
		
		if (d.networkAvailable)
		{
			d.internetAvailable =
			ExecutarComandoSilencioso(
									  "ping.exe -n 1 -w 1000 1.1.1.1");
		}
	}
	
	void AtualizarSeguranca(SystemData& d)
	{
		std::string defender =
		ExecutarComando(
						"powershell.exe -NoProfile -ExecutionPolicy Bypass "
						"-Command \"try { "
						"if ((Get-MpComputerStatus).AntivirusEnabled) { exit 0 } "
						"else { exit 1 } "
						"} catch { exit 2 }\"");
		
		// O código de saída é mais útil que o texto.
		// Reexecutamos silenciosamente para obter o estado.
		std::string comando =
		"powershell.exe -NoProfile -ExecutionPolicy Bypass "
		"-Command \"try { "
		"if ((Get-MpComputerStatus).AntivirusEnabled) { exit 0 } "
		"else { exit 1 } "
		"} catch { exit 2 }\"";
		
		int resultadoDefender =
		std::system(
					("cmd.exe /C " + comando).c_str());
		
		if (resultadoDefender == 0)
		{
			d.defenderAvailable = true;
			d.defenderEnabled = true;
		}
		else
		{
			// Pode significar que o Defender não está disponível
			// ou que está desativado. Mantemos "unavailable" quando
			// não é possível confirmar.
			d.defenderAvailable = false;
			d.defenderEnabled = false;
		}
		
		std::string firewallCommand =
		"powershell.exe -NoProfile -ExecutionPolicy Bypass "
		"-Command \"$p=Get-NetFirewallProfile; "
		"if ($p.Count -gt 0 -and "
		"($p | Where-Object {$_.Enabled -eq $true}).Count -gt 0) "
		"{exit 0} else {exit 1}\"";
		
		int resultadoFirewall =
		std::system(
					("cmd.exe /C " + firewallCommand).c_str());
		
		if (resultadoFirewall == 0)
		{
			d.firewallAvailable = true;
			d.firewallEnabled = true;
		}
		else
		{
			d.firewallAvailable = false;
			d.firewallEnabled = false;
		}
	}
	
	// Atualização completa através dos métodos da própria classe.
	void AtualizarTudo(
					   SystemData& d,
					   SystemMonitor& monitor)
	{
		AtualizarInformacoesSistema(d);
		monitor.Atualizar(d);
		AtualizarDisco(d);
		AtualizarRede(d);
		AtualizarSeguranca(d);
	}
};

// -------------------------------------------------------------
// HealthCalculator
// Responsável pelo cálculo do estado geral do computador.
// -------------------------------------------------------------
class HealthCalculator
{
private:
	// Método privado para mostrar encapsulamento da lógica.
	std::string ObterEstado(int score) const
	{
		if (score >= 85)
			return "GOOD";
		if (score >= 60)
			return "WARNING";
		return "CRITICAL";
	}
	
public:
	void Calcular(SystemData& d)
	{
		int score = 100;
		
		if (d.cpuUsage >= 95)
			score -= 20;
		else if (d.cpuUsage >= 85)
			score -= 10;
		
		if (d.ramUsage >= 95)
			score -= 20;
		else if (d.ramUsage >= 85)
			score -= 10;
		
		if (d.diskUsage >= 95)
			score -= 25;
		else if (d.diskUsage >= 90)
			score -= 15;
		else if (d.diskUsage >= 80)
			score -= 8;
		
		if (!d.networkAvailable)
			score -= 15;
		else if (!d.internetAvailable)
			score -= 8;
		
		if (d.defenderAvailable && !d.defenderEnabled)
			score -= 10;
		
		if (d.firewallAvailable && !d.firewallEnabled)
			score -= 10;
		
		d.healthScore = std::clamp(score, 0, 100);
		
		if (d.healthScore >= 85)
			d.healthStatus = "NORMAL";
		else if (d.healthScore >= 60)
			d.healthStatus = "WARNING";
		else
			d.healthStatus = "CRITICAL";
	}
};

// -------------------------------------------------------------
// DiagnosticEngine
// Executa os testes e cria objetos DiagnosticResult.
// -------------------------------------------------------------
class DiagnosticEngine
{
public:
	std::vector<DiagnosticResult> Executar(
										   const SystemData& d,
										   std::vector<LogEntry>& logs)
	{
		std::vector<DiagnosticResult> resultados;
		
		auto adicionar =
		[&](const std::string& nome,
			const std::string& status,
			const std::string& detalhe,
			int severidade)
		{
			DiagnosticResult r;
			r.name = nome;
			r.status = status;
			r.detail = detalhe;
			r.severity = severidade;
			resultados.push_back(r);
		};
		
		AdicionarLog(logs, "Full System Diagnostic started.");
		
		if (d.cpuUsage >= 95)
			adicionar("CPU", "CRITICAL",
					  "CPU usage is above 95%.", 2);
		else if (d.cpuUsage >= 85)
			adicionar("CPU", "WARNING",
					  "CPU usage is high.", 1);
		else
			adicionar("CPU", "NORMAL",
					  "CPU usage is within normal range.", 0);
		
		if (d.ramUsage >= 95)
			adicionar("RAM", "CRITICAL",
					  "Memory usage is above 95%.", 2);
		else if (d.ramUsage >= 85)
			adicionar("RAM", "WARNING",
					  "Memory usage is high.", 1);
		else
			adicionar("RAM", "NORMAL",
					  "Memory usage is within normal range.", 0);
		
		if (d.diskUsage >= 95)
			adicionar("Storage", "CRITICAL",
					  "C: has less than 5% free space.", 2);
		else if (d.diskUsage >= 90)
			adicionar("Storage", "WARNING",
					  "C: has less than 10% free space.", 1);
		else
			adicionar("Storage", "NORMAL",
					  "C: has sufficient free space.", 0);
		
		if (!d.networkAvailable)
			adicionar("Network", "CRITICAL",
					  "No active network adapter was detected.", 2);
		else if (!d.internetAvailable)
			adicionar("Network", "WARNING",
					  "An active adapter exists, but Internet connectivity could not be confirmed.", 1);
		else
			adicionar("Network", "NORMAL",
					  "Active network adapter and Internet connectivity detected.", 0);
		
		if (!d.defenderAvailable)
			adicionar("Defender", "WARNING",
					  "Windows Defender status could not be confirmed.", 1);
		else if (!d.defenderEnabled)
			adicionar("Defender", "CRITICAL",
					  "Windows Defender antivirus is not confirmed as enabled.", 2);
		else
			adicionar("Defender", "NORMAL",
					  "Windows Defender antivirus is enabled.", 0);
		
		if (!d.firewallAvailable)
			adicionar("Firewall", "WARNING",
					  "Windows Firewall status could not be confirmed.", 1);
		else if (!d.firewallEnabled)
			adicionar("Firewall", "CRITICAL",
					  "At least one Windows Firewall profile is not confirmed as enabled.", 2);
		else
			adicionar("Firewall", "NORMAL",
					  "Windows Firewall has an enabled profile.", 0);
		
		AdicionarLog(
					 logs,
					 "Full System Diagnostic completed with " +
					 std::to_string(resultados.size()) +
					 " checks.");
		
		return resultados;
	}
};

// -------------------------------------------------------------
// ReportManager
// Responsável pela geração dos relatórios técnicos.
// -------------------------------------------------------------
class ReportManager
{
public:
	std::string Gerar(
					  const SystemData& d,
					  const std::vector<DiagnosticResult>& resultados,
					  std::vector<LogEntry>& logs)
	{
		std::string nome =
		"DiagnosticReport_" +
		DataHoraFicheiro() +
		".txt";
		
		std::ofstream ficheiro(nome);
		
		if (!ficheiro)
			return "";
		
		ficheiro
		<< "============================================================\n"
		<< "COMPUTER DIAGNOSTIC CENTER - TECHNICAL REPORT\n"
		<< "============================================================\n\n";
		
		ficheiro << "Generated: " << Agora() << "\n\n";
		
		ficheiro << "[SYSTEM]\n";
		ficheiro << "Computer: " << d.computer << "\n";
		ficheiro << "User: " << d.user << "\n";
		ficheiro << "Architecture: " << d.architecture << "\n";
		ficheiro << "CPU: " << d.cpuModel << "\n";
		ficheiro << "Logical processors: " << d.logicalProcessors << "\n";
		ficheiro << "Uptime: " << d.uptime << "\n\n";
		
		ficheiro << "[PERFORMANCE]\n";
		ficheiro << "CPU usage: " << d.cpuUsage << "%\n";
		ficheiro << "RAM usage: " << d.ramUsage << "%\n";
		ficheiro << "Total RAM: "
		<< std::fixed << std::setprecision(1)
		<< d.totalRamGB << " GB\n";
		ficheiro << "Used RAM: " << d.usedRamGB << " GB\n";
		ficheiro << "Free RAM: " << d.freeRamGB << " GB\n";
		ficheiro << "Processes: " << d.processCount << "\n\n";
		
		ficheiro << "[STORAGE]\n";
		ficheiro << "C: total: " << d.diskTotalGB << " GB\n";
		ficheiro << "C: used: " << d.diskUsedGB << " GB\n";
		ficheiro << "C: free: " << d.diskFreeGB << " GB\n";
		ficheiro << "C: usage: " << d.diskUsage << "%\n\n";
		
		ficheiro << "[NETWORK]\n";
		ficheiro << "Adapter: " << d.adapter << "\n";
		ficheiro << "IP: " << d.localIP << "\n";
		ficheiro << "Gateway: " << d.gateway << "\n";
		ficheiro << "DNS: " << d.dns << "\n";
		ficheiro << "MAC: " << d.mac << "\n";
		ficheiro << "Internet: "
		<< (d.internetAvailable ? "AVAILABLE" : "NOT CONFIRMED")
		<< "\n\n";
		
		ficheiro << "[SECURITY]\n";
		ficheiro << "Defender: "
		<< (d.defenderAvailable
			? (d.defenderEnabled ? "ENABLED" : "DISABLED")
			: "N/A")
		<< "\n";
		
		ficheiro << "Firewall: "
		<< (d.firewallAvailable
			? (d.firewallEnabled ? "ENABLED" : "DISABLED")
			: "N/A")
		<< "\n\n";
		
		ficheiro << "[HEALTH SCORE]\n";
		ficheiro << "Score: " << d.healthScore << "%\n";
		ficheiro << "Status: " << d.healthStatus << "\n\n";
		
		ficheiro << "[DIAGNOSTIC RESULTS]\n";
		
		for (const auto& r : resultados)
		{
			ficheiro << r.name
			<< " | "
			<< r.status
			<< " | "
			<< r.detail
			<< "\n";
		}
		
		ficheiro << "\n============================================================\n";
		ficheiro << "END OF REPORT\n";
		ficheiro << "============================================================\n";
		
		ficheiro.close();
		
		AdicionarLog(
					 logs,
					 "Technical report generated: " + nome);
		
		return nome;
	}
};


// -------------------------------------------------------------
// DiagnosticController
// Reúne os objetos principais da aplicação.
// Demonstra composição: um objeto utiliza vários outros objetos.
// -------------------------------------------------------------
class DiagnosticController
{
private:
	SystemCollector collector;
	HealthCalculator healthCalculator;
	DiagnosticEngine diagnosticEngine;
	ReportManager reportManager;
	
public:
	void Atualizar(
				   SystemData& dados,
				   SystemMonitor& monitor)
	{
		collector.AtualizarTudo(dados, monitor);
		healthCalculator.Calcular(dados);
	}
	
	void AtualizarEmTempoReal(
							  SystemData& dados,
							  SystemMonitor& monitor)
	{
		monitor.Atualizar(dados);
		collector.AtualizarDisco(dados);
		healthCalculator.Calcular(dados);
	}
	
	std::vector<DiagnosticResult> ExecutarDiagnostico(
													  const SystemData& dados,
													  std::vector<LogEntry>& logs)
	{
		return diagnosticEngine.Executar(dados, logs);
	}
	
	std::string GerarRelatorio(
							   const SystemData& dados,
							   const std::vector<DiagnosticResult>& resultados,
							   std::vector<LogEntry>& logs)
	{
		return reportManager.Gerar(dados, resultados, logs);
	}
};

// =============================================================
// INTERFACE
// =============================================================

void DesenharTextoCentrado(
						   Font fonte,
						   const char* texto,
						   Rectangle area,
						   float tamanho,
						   Color cor)
{
	Vector2 medidas =
	MeasureTextEx(fonte, texto, tamanho, 1);
	
	DrawTextEx(
			   fonte,
			   texto,
			   {
				   area.x + (area.width - medidas.x) / 2.0f,
				   area.y + (area.height - medidas.y) / 2.0f
			   },
			   tamanho,
			   1,
			   cor);
}

bool DesenharBotao(
				   Font fonte,
				   Rectangle botao,
				   const char* texto,
				   float tamanho = 17)
{
	bool sobre =
	CheckCollisionPointRec(
						   GetMousePosition(),
						   botao);
	
	Color cor = sobre
	? COR_BOTAO_HOVER
	: COR_BOTAO;
	
	if (sobre && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		cor = COR_BOTAO_PREMIDO;
	
	DrawRectangleRounded(
						 botao,
						 0.12f,
						 8,
						 cor);
	
	DesenharTextoCentrado(
						  fonte,
						  texto,
						  botao,
						  tamanho,
						  WHITE);
	
	return sobre &&
	IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void Painel(
			Rectangle r,
			const char* titulo,
			Font fonte)
{
	DrawRectangleRounded(
						 r,
						 0.04f,
						 8,
						 COR_PAINEL);
	
	DrawRectangleLinesEx(
						 r,
						 1,
						 COR_CONTORNO);
	
	DrawTextEx(
			   fonte,
			   titulo,
			   {r.x + 18, r.y + 14},
			   19,
			   1,
			   WHITE);
}

void Texto(
		   Font fonte,
		   const std::string& texto,
		   float x,
		   float y,
		   float tamanho = 17,
		   Color cor = COR_TEXTO)
{
	DrawTextEx(
			   fonte,
			   texto.c_str(),
			   {x, y},
			   tamanho,
			   1,
			   cor);
}

void Barra(
		   Font fonte,
		   float x,
		   float y,
		   float largura,
		   float valor,
		   const char* label)
{
	Texto(
		  fonte,
		  std::string(label) + " " +
		  std::to_string(static_cast<int>(valor)) + "%",
		  x,
		  y,
		  15,
		  COR_TEXTO);
	
	Rectangle fundo{
		x,
		y + 24,
		largura,
		10
	};
	
	DrawRectangleRounded(
						 fundo,
						 0.5f,
						 8,
						 {25, 45, 48, 255});
	
	Rectangle preenchido{
		x,
		y + 24,
		largura * std::clamp(valor / 100.0f, 0.0f, 1.0f),
		10
	};
	
	Color cor = COR_VERDE;
	
	if (valor >= 90)
		cor = COR_VERMELHO;
	else if (valor >= 80)
		cor = COR_AMARELO;
	
	DrawRectangleRounded(
						 preenchido,
						 0.5f,
						 8,
						 cor);
}

// =============================================================
// MENU LATERAL
// =============================================================

Page DesenharMenu(
				  Font fonte,
				  Page paginaAtual)
{
	DrawRectangle(
				  0,
				  0,
				  LARGURA_MENU,
				  ALTURA_JANELA,
				  COR_CABECALHO);
	
	// Logo Kj no cabeçalho do menu lateral.
	if (LOGO_KJ_VALIDO)
	{
		Rectangle origem{
			0.0f, 0.0f,
			static_cast<float>(LOGO_KJ.width),
			static_cast<float>(LOGO_KJ.height)
		};
		
		Rectangle destino{
			16.0f, 10.0f, 62.0f, 62.0f
		};
		
		DrawTexturePro(
					   LOGO_KJ, origem, destino,
					   {0.0f, 0.0f}, 0.0f, WHITE);
	}
	
	Texto(
		  fonte,
		  "COMPUTER",
		  88, 18, 17, COR_TITULO);
	
	Texto(
		  fonte,
		  "DIAGNOSTIC CENTER",
		  88, 43, 14, COR_TEXTO);
	
	Texto(
		  fonte,
		  "PROFESSIONAL",
		  88, 64, 10, COR_TEXTO_DIM);
	
	const char* nomes[] =
	{
		"DASHBOARD",
		"DIAGNOSTICS",
		"NETWORK",
		"HARDWARE",
		"STORAGE",
		"SECURITY",
		"ADMINISTRATION",
		"REPORTS"
	};
	
	Page paginas[] =
	{
		Page::DASHBOARD,
		Page::DIAGNOSTICS,
		Page::NETWORK,
		Page::HARDWARE,
		Page::STORAGE,
		Page::SECURITY,
		Page::ADMINISTRATION,
		Page::REPORTS
	};
	
	for (int i = 0; i < 8; ++i)
	{
		Rectangle r{
			15.0f,
			105.0f + i * 55.0f,
			215.0f,
			43.0f
		};
		
		bool ativo =
		paginaAtual == paginas[i];
		
		if (ativo)
			DrawRectangleRounded(
								 r,
								 0.12f,
								 8,
								 COR_BOTAO_HOVER);
		
		if (DesenharBotao(
						  fonte,
						  r,
						  nomes[i],
						  15))
		{
			return paginas[i];
		}
	}
	
	return paginaAtual;
}

// =============================================================
// PÁGINAS
// =============================================================

void DesenharDashboard(
					   Font fonte,
					   const SystemData& d,
					   const std::vector<LogEntry>& logs,
					   bool& executarDiagnostico,
					   bool& atualizarAgora)
{
	Texto(
		  fonte,
		  "SYSTEM DASHBOARD",
		  275,
		  25,
		  28,
		  COR_TITULO);
	
	Texto(
		  fonte,
		  "Real-time system overview",
		  275,
		  58,
		  15,
		  COR_TEXTO_DIM);
	
	// Health Score
	Rectangle health{
		275,
		95,
		275,
		190
	};
	
	Painel(health, "SYSTEM HEALTH", fonte);
	
	Texto(
		  fonte,
		  std::to_string(d.healthScore) + "%",
		  300,
		  135,
		  52,
		  CorEstado(
					d.healthScore < 60 ? 2 :
					d.healthScore < 85 ? 1 : 0));
	
	Texto(
		  fonte,
		  d.healthStatus,
		  305,
		  205,
		  19,
		  COR_TEXTO);
	
	// Performance
	Rectangle perf{
		570,
		95,
		570,
		190
	};
	
	Painel(perf, "PERFORMANCE", fonte);
	
	Barra(
		  fonte,
		  595,
		  135,
		  220,
		  d.cpuUsage,
		  "CPU");
	
	Barra(
		  fonte,
		  595,
		  190,
		  220,
		  d.ramUsage,
		  "RAM");
	
	Barra(
		  fonte,
		  865,
		  135,
		  220,
		  d.diskUsage,
		  "DISK");
	
	Texto(
		  fonte,
		  "Processes: " + std::to_string(d.processCount),
		  865,
		  200,
		  16,
		  COR_TEXTO);
	
	// System
	Rectangle system{
		275,
		305,
		420,
		250
	};
	
	Painel(system, "SYSTEM INFORMATION", fonte);
	
	Texto(fonte, "Computer: " + d.computer, 300, 350);
	Texto(fonte, "User: " + d.user, 300, 385);
	Texto(fonte, "CPU: " + d.cpuModel, 300, 420, 15);
	Texto(fonte, "Architecture: " + d.architecture, 300, 455);
	Texto(fonte, "Logical processors: " +
		  std::to_string(d.logicalProcessors), 300, 490);
	Texto(fonte, "Uptime: " + d.uptime, 300, 525, 15);
	
	// Network
	Rectangle network{
		720,
		305,
		420,
		250
	};
	
	Painel(network, "NETWORK STATUS", fonte);
	
	Texto(
		  fonte,
		  "Adapter: " + d.adapter,
		  745,
		  350,
		  15);
	
	Texto(
		  fonte,
		  "IP: " + d.localIP,
		  745,
		  385);
	
	Texto(
		  fonte,
		  "Gateway: " + d.gateway,
		  745,
		  420);
	
	Texto(
		  fonte,
		  "DNS: " + d.dns,
		  745,
		  455);
	
	Texto(
		  fonte,
		  std::string("Internet: ") +
		  (d.internetAvailable ? "AVAILABLE" : "NOT CONFIRMED"),
		  745,
		  500,
		  18,
		  d.internetAvailable ? COR_VERDE : COR_AMARELO);
	
	// Quick actions
	Texto(
		  fonte,
		  "QUICK DIAGNOSTICS",
		  275,
		  595,
		  20,
		  WHITE);
	
	Rectangle run{
		275,
		630,
		280,
		55
	};
	
	if (DesenharBotao(
					  fonte,
					  run,
					  "RUN FULL DIAGNOSTIC",
					  16))
	{
		executarDiagnostico = true;
	}
	
	Rectangle refresh{
		575,
		630,
		230,
		55
	};
	
	if (DesenharBotao(
					  fonte,
					  refresh,
					  "REFRESH DATA",
					  16))
	{
		atualizarAgora = true;
	}
	
	Texto(
		  fonte,
		  "Logs recorded: " +
		  std::to_string(logs.size()),
		  835,
		  650,
		  15,
		  COR_TEXTO_DIM);
}

void DesenharDiagnostics(
						 Font fonte,
						 const std::vector<DiagnosticResult>& resultados,
						 bool& executarDiagnostico)
{
	Texto(
		  fonte,
		  "FULL SYSTEM DIAGNOSTIC",
		  275,
		  25,
		  28,
		  COR_TITULO);
	
	Texto(
		  fonte,
		  "Analyse real system data and identify warnings",
		  275,
		  58,
		  15,
		  COR_TEXTO_DIM);
	
	Rectangle run{
		275,
		90,
		280,
		50
	};
	
	if (DesenharBotao(
					  fonte,
					  run,
					  "RUN DIAGNOSTIC",
					  16))
	{
		executarDiagnostico = true;
	}
	
	int y = 165;
	
	if (resultados.empty())
	{
		Texto(
			  fonte,
			  "No diagnostic has been executed yet.",
			  300,
			  185,
			  18,
			  COR_TEXTO_DIM);
		return;
	}
	
	for (const auto& r : resultados)
	{
		Rectangle row{
			275,
			static_cast<float>(y),
			850,
			60
		};
		
		DrawRectangleRounded(
							 row,
							 0.05f,
							 8,
							 COR_PAINEL);
		
		DrawRectangleLinesEx(
							 row,
							 1,
							 CorEstado(r.severity));
		
		Texto(
			  fonte,
			  r.name,
			  295,
			  y + 10,
			  17,
			  WHITE);
		
		Texto(
			  fonte,
			  r.status,
			  520,
			  y + 10,
			  16,
			  CorEstado(r.severity));
		
		Texto(
			  fonte,
			  r.detail,
			  295,
			  y + 35,
			  13,
			  COR_TEXTO_DIM);
		
		y += 70;
		
		if (y > 745)
			break;
	}
}

void DesenharNetwork(
					 Font fonte,
					 const SystemData& d)
{
	Texto(
		  fonte,
		  "NETWORK DIAGNOSTICS",
		  275,
		  25,
		  28,
		  COR_TITULO);
	
	Rectangle info{
		275,
		90,
		550,
		350
	};
	
	Painel(info, "NETWORK INFORMATION", fonte);
	
	Texto(fonte, "Adapter: " + d.adapter, 300, 140, 16);
	Texto(fonte, "Local IP: " + d.localIP, 300, 180);
	Texto(fonte, "Gateway: " + d.gateway, 300, 220);
	Texto(fonte, "DNS: " + d.dns, 300, 260);
	Texto(fonte, "MAC: " + d.mac, 300, 300);
	Texto(
		  fonte,
		  std::string("Adapter status: ") +
		  (d.networkAvailable ? "UP" : "DOWN / N/A"),
		  300,
		  340,
		  17,
		  d.networkAvailable ? COR_VERDE : COR_VERMELHO);
	
	Texto(
		  fonte,
		  std::string("Internet: ") +
		  (d.internetAvailable ? "AVAILABLE" : "NOT CONFIRMED"),
		  300,
		  380,
		  17,
		  d.internetAvailable ? COR_VERDE : COR_AMARELO);
	
	Rectangle tools{
		850,
		90,
		290,
		350
	};
	
	Painel(tools, "NETWORK TOOLS", fonte);
	
	if (DesenharBotao(
					  fonte,
					  {875, 145, 240, 45},
					  "NETWORK CONNECTIONS",
					  14))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "control.exe",
					  "ncpa.cpl",
					  nullptr,
					  SW_SHOWNORMAL);
	}
	
	if (DesenharBotao(
					  fonte,
					  {875, 205, 240, 45},
					  "COMMAND PROMPT",
					  14))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "cmd.exe",
					  nullptr,
					  nullptr,
					  SW_SHOWNORMAL);
	}
	
	if (DesenharBotao(
					  fonte,
					  {875, 265, 240, 45},
					  "POWERSHELL",
					  14))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "powershell.exe",
					  nullptr,
					  nullptr,
					  SW_SHOWNORMAL);
	}
	
	if (DesenharBotao(
					  fonte,
					  {875, 325, 240, 45},
					  "IPCONFIG",
					  14))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "cmd.exe",
					  "/K ipconfig /all",
					  nullptr,
					  SW_SHOWNORMAL);
	}
}

void DesenharHardware(
					  Font fonte,
					  const SystemData& d)
{
	Texto(
		  fonte,
		  "HARDWARE DIAGNOSTICS",
		  275,
		  25,
		  28,
		  COR_TITULO);
	
	Rectangle cpu{
		275,
		90,
		420,
		230
	};
	
	Painel(cpu, "CPU", fonte);
	
	Texto(
		  fonte,
		  "Model: " + d.cpuModel,
		  300,
		  140,
		  15);
	
	Texto(
		  fonte,
		  "Logical processors: " +
		  std::to_string(d.logicalProcessors),
		  300,
		  180);
	
	Texto(
		  fonte,
		  "Current usage: " +
		  std::to_string(d.cpuUsage) + "%",
		  300,
		  220);
	
	Texto(
		  fonte,
		  "Architecture: " + d.architecture,
		  300,
		  260);
	
	Rectangle ram{
		720,
		90,
		420,
		230
	};
	
	Painel(ram, "MEMORY", fonte);
	
	Texto(
		  fonte,
		  "Total: " +
		  std::to_string(
						 static_cast<int>(d.totalRamGB)) +
						 " GB",
						 745,
						 140);
	
	Texto(
		  fonte,
		  "Used: " +
		  std::to_string(
						 static_cast<int>(d.usedRamGB)) +
						 " GB",
						 745,
						 180);
	
	Texto(
		  fonte,
		  "Free: " +
		  std::to_string(
						 static_cast<int>(d.freeRamGB)) +
						 " GB",
						 745,
						 220);
	
	Texto(
		  fonte,
		  "Usage: " +
		  std::to_string(d.ramUsage) + "%",
		  745,
		  260);
	
	Rectangle tools{
		275,
		350,
		865,
		180
	};
	
	Painel(tools, "WINDOWS HARDWARE TOOLS", fonte);
	
	if (DesenharBotao(
					  fonte,
					  {300, 405, 250, 50},
					  "DEVICE MANAGER"))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "devmgmt.msc",
					  nullptr,
					  nullptr,
					  SW_SHOWNORMAL);
	}
	
	if (DesenharBotao(
					  fonte,
					  {570, 405, 250, 50},
					  "SYSTEM INFORMATION"))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "msinfo32.exe",
					  nullptr,
					  nullptr,
					  SW_SHOWNORMAL);
	}
	
	if (DesenharBotao(
					  fonte,
					  {840, 405, 250, 50},
					  "TASK MANAGER"))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "taskmgr.exe",
					  nullptr,
					  nullptr,
					  SW_SHOWNORMAL);
	}
}

void DesenharStorage(
					 Font fonte,
					 const SystemData& d)
{
	Texto(
		  fonte,
		  "STORAGE ANALYZER",
		  275,
		  25,
		  28,
		  COR_TITULO);
	
	Rectangle disk{
		275,
		90,
		865,
		260
	};
	
	Painel(disk, "SYSTEM DRIVE C:", fonte);
	
	Texto(
		  fonte,
		  "Total: " +
		  std::to_string(d.diskTotalGB) +
		  " GB",
		  305,
		  145);
	
	Texto(
		  fonte,
		  "Used: " +
		  std::to_string(d.diskUsedGB) +
		  " GB",
		  305,
		  185);
	
	Texto(
		  fonte,
		  "Free: " +
		  std::to_string(d.diskFreeGB) +
		  " GB",
		  305,
		  225);
	
	Texto(
		  fonte,
		  "Usage: " +
		  std::to_string(d.diskUsage) +
		  "%",
		  305,
		  265,
		  20,
		  d.diskUsage >= 90
		  ? COR_VERMELHO
		  : d.diskUsage >= 80
		  ? COR_AMARELO
		  : COR_VERDE);
	
	Barra(
		  fonte,
		  520,
		  150,
		  550,
		  d.diskUsage,
		  "C:");
	
	if (DesenharBotao(
					  fonte,
					  {520, 220, 250, 50},
					  "DISK CLEANUP"))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "cleanmgr.exe",
					  nullptr,
					  nullptr,
					  SW_SHOWNORMAL);
	}
	
	if (DesenharBotao(
					  fonte,
					  {800, 220, 250, 50},
					  "DISK MANAGEMENT"))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "diskmgmt.msc",
					  nullptr,
					  nullptr,
					  SW_SHOWNORMAL);
	}
	
	Texto(
		  fonte,
		  "The application does not automatically delete files.",
		  300,
		  390,
		  16,
		  COR_TEXTO_DIM);
	
	Texto(
		  fonte,
		  "Use Disk Cleanup to review Windows-recommended cleanup actions.",
		  300,
		  420,
		  14,
		  COR_TEXTO_DIM);
}

void DesenharSecurity(
					  Font fonte,
					  const SystemData& d)
{
	Texto(
		  fonte,
		  "SECURITY CENTER",
		  275,
		  25,
		  28,
		  COR_TITULO);
	
	Rectangle sec{
		275,
		90,
		865,
		300
	};
	
	Painel(sec, "SECURITY STATUS", fonte);
	
	Texto(
		  fonte,
		  "Windows Defender",
		  305,
		  145,
		  18,
		  WHITE);
	
	Texto(
		  fonte,
		  d.defenderAvailable
		  ? (d.defenderEnabled ? "ENABLED" : "DISABLED")
		  : "N/A",
		  600,
		  145,
		  18,
		  d.defenderAvailable
		  ? (d.defenderEnabled ? COR_VERDE : COR_VERMELHO)
		  : COR_AMARELO);
	
	Texto(
		  fonte,
		  "Windows Firewall",
		  305,
		  205,
		  18,
		  WHITE);
	
	Texto(
		  fonte,
		  d.firewallAvailable
		  ? (d.firewallEnabled ? "ENABLED" : "DISABLED")
		  : "N/A",
		  600,
		  205,
		  18,
		  d.firewallAvailable
		  ? (d.firewallEnabled ? COR_VERDE : COR_VERMELHO)
		  : COR_AMARELO);
	
	Texto(
		  fonte,
		  "Security score is based only on checks the application can confirm.",
		  305,
		  275,
		  14,
		  COR_TEXTO_DIM);
	
	if (DesenharBotao(
					  fonte,
					  {305, 320, 250, 45},
					  "WINDOWS SECURITY"))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "windowsdefender:",
					  nullptr,
					  nullptr,
					  SW_SHOWNORMAL);
	}
	
	if (DesenharBotao(
					  fonte,
					  {580, 320, 250, 45},
					  "FIREWALL"))
	{
		ShellExecuteA(
					  nullptr,
					  "open",
					  "control.exe",
					  "firewall.cpl",
					  nullptr,
					  SW_SHOWNORMAL);
	}
}

void DesenharAdministration(
							Font fonte)
{
	Texto(
		  fonte,
		  "SYSTEM ADMINISTRATION",
		  275,
		  25,
		  28,
		  COR_TITULO);
	
	struct Tool
	{
		const char* nome;
		const char* programa;
		const char* argumentos;
	};
	
	Tool tools[] =
	{
		{"TASK MANAGER", "taskmgr.exe", nullptr},
		{"SERVICES", "services.msc", nullptr},
		{"EVENT VIEWER", "eventvwr.msc", nullptr},
		{"TASK SCHEDULER", "taskschd.msc", nullptr},
		{"COMPUTER MANAGEMENT", "compmgmt.msc", nullptr},
		{"CONTROL PANEL", "control.exe", nullptr},
		{"POWERSHELL", "powershell.exe", nullptr},
		{"COMMAND PROMPT", "cmd.exe", nullptr}
	};
	
	for (int i = 0; i < 8; ++i)
	{
		int coluna = i % 2;
		int linha = i / 2;
		
		Rectangle r{
			275.0f + coluna * 440.0f,
			95.0f + linha * 85.0f,
			400,
			58
		};
		
		if (DesenharBotao(
						  fonte,
						  r,
						  tools[i].nome,
						  16))
		{
			ShellExecuteA(
						  nullptr,
						  "open",
						  tools[i].programa,
						  tools[i].argumentos,
						  nullptr,
						  SW_SHOWNORMAL);
		}
	}
	
	Rectangle note{
		275,
		470,
		840,
		100
	};
	
	Painel(note, "ADMINISTRATION NOTE", fonte);
	
	Texto(
		  fonte,
		  "Administrative actions are opened in the native Windows tools.",
		  300,
		  510,
		  15,
		  COR_TEXTO);
	
	Texto(
		  fonte,
		  "The application does not silently change services, users or system configuration.",
		  300,
		  540,
		  13,
		  COR_TEXTO_DIM);
}

void DesenharReports(
					 Font fonte,
					 const std::vector<DiagnosticResult>& resultados,
					 const SystemData& d,
					 std::vector<LogEntry>& logs,
					 std::string& ultimoRelatorio,
					 DiagnosticController& controller)
{
	Texto(
		  fonte,
		  "REPORTS & LOGS",
		  275,
		  25,
		  28,
		  COR_TITULO);
	
	Rectangle report{
		275,
		90,
		865,
		160
	};
	
	Painel(report, "TECHNICAL REPORT", fonte);
	
	if (DesenharBotao(
					  fonte,
					  {300, 145, 280, 50},
					  "GENERATE REPORT",
					  16))
	{
		ultimoRelatorio =
		controller.GerarRelatorio(
								  d,
								  resultados,
								  logs);
	}
	
	if (!ultimoRelatorio.empty())
	{
		Texto(
			  fonte,
			  "Last report: " + ultimoRelatorio,
			  300,
			  215,
			  14,
			  COR_VERDE);
	}
	else
	{
		Texto(
			  fonte,
			  "No report generated during this session.",
			  300,
			  215,
			  14,
			  COR_TEXTO_DIM);
	}
	
	Rectangle logPanel{
		275,
		280,
		865,
		430
	};
	
	Painel(logPanel, "DIAGNOSTIC LOG", fonte);
	
	int inicio =
	logs.size() > 11
	? static_cast<int>(logs.size()) - 11
	: 0;
	
	int y = 325;
	
	for (int i = inicio;
		 i < static_cast<int>(logs.size());
		 ++i)
	{
		Texto(
			  fonte,
			  "[" + logs[i].time + "]",
			  300,
			  y,
			  13,
			  COR_TITULO);
		
		Texto(
			  fonte,
			  logs[i].message,
			  485,
			  y,
			  13,
			  COR_TEXTO);
		
		y += 31;
	}
}

// =============================================================
// MAIN
// =============================================================

int main()
{
	InitWindow(
			   LARGURA_JANELA,
			   ALTURA_JANELA,
			   "Computer Diagnostic Center - Professional");
	
	SetTargetFPS(60);
	
	// ---------------------------------------------------------
	// Logo Kj: ícone da janela + logo visível na interface
	// ---------------------------------------------------------
	Image icone = LoadImage("kj_logo.png");
	
	if (icone.data)
	{
		SetWindowIcon(icone);
		LOGO_KJ = LoadTextureFromImage(icone);
		LOGO_KJ_VALIDO = (LOGO_KJ.id != 0);
		UnloadImage(icone);
	}
	
	Font fonte =
	LoadFontEx(
			   "C:/Windows/Fonts/segoeui.ttf",
			   32,
			   nullptr,
			   512);
	
	if (fonte.texture.id != 0)
		SetTextureFilter(
						 fonte.texture,
						 TEXTURE_FILTER_BILINEAR);
	
	SystemData dados;
	SystemMonitor monitor;
	
	// Objeto principal que agrega os serviços da aplicação.
	DiagnosticController controller;
	
	std::vector<LogEntry> logs;
	std::vector<DiagnosticResult> resultados;
	
	Page pagina = Page::DASHBOARD;
	
	bool executarDiagnostico = false;
	bool atualizarAgora = false;
	std::string ultimoRelatorio;
	
	controller.Atualizar(dados, monitor);
	
	AdicionarLog(
				 logs,
				 "Application started.");
	
	double ultimaAtualizacao =
	GetTime();
	
	while (!WindowShouldClose())
	{
		// ---------------------------------------------------------
		// Atualização automática
		// ---------------------------------------------------------
		
		if (GetTime() - ultimaAtualizacao >= 1.0)
		{
			// Para não executar PowerShell constantemente,
			// atualizamos os dados principais em tempo real.
			controller.AtualizarEmTempoReal(dados, monitor);
			
			ultimaAtualizacao = GetTime();
		}
		
		// ---------------------------------------------------------
		// REFRESH / FULL DIAGNOSTIC
		// ---------------------------------------------------------
		
		if (atualizarAgora)
		{
			controller.Atualizar(dados, monitor);
			
			AdicionarLog(
						 logs,
						 "Manual data refresh completed.");
			
			atualizarAgora = false;
		}
		
		if (executarDiagnostico)
		{
			// A recolha completa ocorre uma vez quando solicitada.
			controller.Atualizar(dados, monitor);
			
			resultados =
			controller.ExecutarDiagnostico(
										   dados,
										   logs);
			
			executarDiagnostico = false;
		}
		
		// ---------------------------------------------------------
		// DESENHO
		// ---------------------------------------------------------
		
		BeginDrawing();
		
		ClearBackground(COR_FUNDO);
		
		Page novaPagina =
		DesenharMenu(
					 fonte,
					 pagina);
		
		pagina = novaPagina;
		
		// Área principal
		DrawRectangle(
					  LARGURA_MENU,
					  0,
					  LARGURA_JANELA - LARGURA_MENU,
					  ALTURA_JANELA,
					  COR_FUNDO);
		
		switch (pagina)
		{
		case Page::DASHBOARD:
			DesenharDashboard(
							  fonte,
							  dados,
							  logs,
							  executarDiagnostico,
							  atualizarAgora);
			break;
			
		case Page::DIAGNOSTICS:
			DesenharDiagnostics(
								fonte,
								resultados,
								executarDiagnostico);
			break;
			
		case Page::NETWORK:
			DesenharNetwork(
							fonte,
							dados);
			break;
			
		case Page::HARDWARE:
			DesenharHardware(
							 fonte,
							 dados);
			break;
			
		case Page::STORAGE:
			DesenharStorage(
							fonte,
							dados);
			break;
			
		case Page::SECURITY:
			DesenharSecurity(
							 fonte,
							 dados);
			break;
			
		case Page::ADMINISTRATION:
			DesenharAdministration(
								   fonte);
			break;
			
		case Page::REPORTS:
			DesenharReports(
							fonte,
							resultados,
							dados,
							logs,
							ultimoRelatorio,
							controller);
			break;
		}
		
		// ---------------------------------------------------------
		// Barra inferior
		// ---------------------------------------------------------
		
		DrawLine(
				 LARGURA_MENU + 20,
				 ALTURA_JANELA - 42,
				 LARGURA_JANELA - 20,
				 ALTURA_JANELA - 42,
				 {35, 65, 68, 255});
		
		Texto(
			  fonte,
			  "LIVE",
			  LARGURA_MENU + 25,
			  ALTURA_JANELA - 30,
			  13,
			  COR_VERDE);
		
		Texto(
			  fonte,
			  "Last update: " + Agora(),
			  LARGURA_MENU + 85,
			  ALTURA_JANELA - 30,
			  13,
			  COR_TEXTO_DIM);
		
		Texto(
			  fonte,
			  "Health: " +
			  std::to_string(dados.healthScore) + "%",
			  LARGURA_JANELA - 170,
			  ALTURA_JANELA - 30,
			  13,
			  CorEstado(
						dados.healthScore < 60 ? 2 :
						dados.healthScore < 85 ? 1 : 0));
		
		EndDrawing();
	}
	
	AdicionarLog(
				 logs,
				 "Application closed.");
	
	if (fonte.texture.id != 0)
		UnloadFont(fonte);
	
	if (LOGO_KJ_VALIDO)
		UnloadTexture(LOGO_KJ);
	
	CloseWindow();
	
	return 0;
}
