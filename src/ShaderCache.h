#pragma once

#include <RE/B/BSShader.h>

#include <condition_variable>
#include <unordered_set>
#include <unordered_map>

namespace SIE
{
	enum class ShaderClass
	{
		Vertex,
		Pixel,
		Compute,
		Total,
	};

	class ShaderCompilationTask
	{
	public:
		ShaderCompilationTask(ShaderClass shaderClass, const RE::BSShader& shader,
			uint32_t descriptor);
		void Perform() const;

		size_t GetId() const;

		bool operator==(const ShaderCompilationTask& other) const;

	protected:
		ShaderClass shaderClass;
		const RE::BSShader& shader;
		uint32_t descriptor;
	};
}

template <>
struct std::hash<SIE::ShaderCompilationTask>
{
	std::size_t operator()(const SIE::ShaderCompilationTask& task) const noexcept
	{
		return task.GetId();
	}
};

namespace SIE
{
	class CompilationSet
	{
	public:
		ShaderCompilationTask WaitTake();
		void Add(const ShaderCompilationTask& task);
		void Complete(const ShaderCompilationTask& task);
		void Clear();
		std::atomic<uint64_t> completedTasks = 0;
		std::atomic<uint64_t> totalTasks = 0;
		std::mutex mutex;
	private:
		std::unordered_set<ShaderCompilationTask> availableTasks;
		std::unordered_set<ShaderCompilationTask> tasksInProgress;
		std::condition_variable conditionVariable;
	};

	class ShaderCache
	{
	public:
		static ShaderCache& Instance()
		{
			static ShaderCache instance;
			return instance;
		}

		bool IsEnabled() const;
		void SetEnabled(bool value);
		bool IsEnabledForClass(ShaderClass shaderClass) const;
		void SetEnabledForClass(ShaderClass shaderClass, bool value);
		bool IsAsync() const;
		void SetAsync(bool value);

		bool IsDiskCache() const;
		void SetDiskCache(bool value);
		void DeleteDiskCache();

		void Clear();

		RE::BSGraphics::VertexShader* GetVertexShader(const RE::BSShader& shader, uint32_t descriptor);
		RE::BSGraphics::PixelShader* GetPixelShader(const RE::BSShader& shader,
			uint32_t descriptor);

		RE::BSGraphics::VertexShader* MakeAndAddVertexShader(const RE::BSShader& shader,
			uint32_t descriptor);
		RE::BSGraphics::PixelShader* MakeAndAddPixelShader(const RE::BSShader& shader,
			uint32_t descriptor);

		uint64_t GetCompletedTasks();
		uint64_t GetTotalTasks();

	private:
		ShaderCache();
		void ProcessCompilationSet();

		~ShaderCache();

		std::array<std::map<uint32_t, std::unique_ptr<RE::BSGraphics::VertexShader>>,
			static_cast<size_t>(RE::BSShader::Type::Total)>
			vertexShaders;
		std::array<std::map<uint32_t, std::unique_ptr<RE::BSGraphics::PixelShader>>,
			static_cast<size_t>(RE::BSShader::Type::Total)>
			pixelShaders;

		bool isEnabled = false;
		bool isDiskCache = false;

		uint32_t disabledClasses = 0;

		bool isAsync = true;
		std::vector<std::jthread> compilationThreads;
		std::mutex vertexShadersMutex;
		std::mutex pixelShadersMutex;
		CompilationSet compilationSet;

		RE::BSShader* currentShader = nullptr;
		uint32_t currentDescriptor = 0;


	};
}
