# Chrome Enterprise Architecture Design

*This document outlines the existing enterprise architecture and design patterns within the upstream Chromium project. It serves as a reference for understanding how enterprise features are implemented in Chromium, which is the foundation for our custom enterprise browser.*

## 1. Overall Architectural Overview

Chromium's enterprise features are designed in a modular and layered manner, adhering to the browser's core architectural principles. The investigation reveals a clear pattern:

-   **Layered Design**: The foundational, generic enterprise logic resides in the `components/` directory (e.g., `components/policy/`, `components/enterprise/`). The browser-specific implementations and UI-related code that use these components are located in `chrome/browser/enterprise/`. This separation is critical for maintainability and code sharing.

-   **Keyed Services**: Most enterprise features are implemented as `KeyedService`s. This is a standard Chromium pattern for features that are tied to a specific `Profile` (or `BrowserContext`). It ensures that feature instances are created, managed, and destroyed along with user profiles, providing a clean lifecycle and separation of data between users.

-   **Manager/Service Separation**: A common design pattern observed is the separation of a feature's public API from its internal logic.
    -   A `[Feature]Service` class often acts as the `KeyedService`, providing a clean, stable interface for other parts of the browser to consume.
    -   A `[Feature]Manager` class typically contains the complex logic of reading, parsing, and caching enterprise policies, often from `PrefService`. The `Service` class owns the `Manager`. This insulates the rest of the browser from the complexities of policy management.

-   **Policy-Driven**: All features are fundamentally driven by enterprise policies. They are designed to be configured remotely by an administrator via the policy system.

---

## 2. Component Breakdown (`chrome/browser/enterprise/`)

### 2.1 `browser_management/`

-   **Purpose**: This component's primary responsibility is to determine if the browser is managed by an organization and to provide this status to other browser components.
-   **Design**:
    -   The main entry point is the `BrowserManagementService`, a `KeyedService` that provides the management status.
    -   It uses a collection of `ManagementStatusProvider` objects, each designed to detect a specific type of management signal (e.g., `BrowserCloudManagementStatusProvider` for cloud policy, `LocalBrowserManagementStatusProvider` for domain membership on the host OS).
    -   This modular design allows new types of management signals to be added easily without changing the core service. The architecture is detailed in `components/policy/core/common/management/management_service.md`.

### 2.2 `connectors/`

-   **Purpose**: Implements the "Enterprise Connectors" framework. This framework acts as a bridge between the browser and **external services** (which can include both third-party vendors and custom, first-party backends) for security, data protection, and content analysis. As noted in its `README.md`, it integrates deeply with Safe Browsing for real-time content scanning.
-   **Design**:
    -   **`ConnectorsService`**: The public-facing `KeyedService` that provides connector-related settings (e.g., "Should I scan this download?" or "Where should I report this security event?").
    -   **`ConnectorsManager`**: The internal engine that handles the complex logic of reading and interpreting the various connector policies from `PrefService`. It caches these policies and manages their lifecycle. This separation keeps the `ConnectorsService` clean and focused on serving settings to its clients.
    -   The framework is designed to be extensible, allowing different types of connectors (e.g., for reporting, content analysis) to be managed through the same central system.

### 2.3 `core/`

-   **Purpose**: This directory provides a critical piece of infrastructure that enables clean architecture for other enterprise features. It is not a feature itself, but an implementation of the Dependency Inversion Principle to decouple reusable components from the browser-specific `Profile` object.
-   **Design**:
    -   **Abstract Factory**: The core contract is the abstract `enterprise_core::DependencyFactory` class located in `components/enterprise/core/`. This interface defines methods for retrieving dependencies (e.g., `GetUserCloudPolicyManager()`) without exposing any browser-specific details.
    -   **Concrete Implementation**: This directory (`chrome/browser/enterprise/core/`) contains the `DependencyFactoryImpl`. This concrete class takes a `Profile*` in its constructor and implements the abstract methods by calling the appropriate methods on the `Profile` object (e.g., `profile->GetCloudPolicyManager()`).
    -   **Decoupling**: By coding reusable features in `components/` against the abstract `DependencyFactory`, those features can be tested in isolation by injecting a `MockDependencyFactory`. This avoids a direct, hard dependency on the `Profile` object, making the code more modular and maintainable.

### 2.4 `data_controls/`

-   **Purpose**: Implements Chrome's Data Loss Prevention (DLP) functionality. Its purpose is to enforce enterprise policies that restrict data transfer actions like clipboard usage, printing, and screen sharing based on the context of the source and destination.
-   **Design**:
    -   **`ChromeRulesService`**: This per-profile `KeyedService` is the main entry point for the DLP system. Other browser components call its methods (e.g., `GetPasteVerdict`) to check if an action is allowed.
    -   **Policy and Rules**: The feature is configured by the `DataControlsRules` JSON enterprise policy. The `RulesServiceBase` (a component-level class) listens for policy changes and parses the JSON into a vector of `Rule` objects.
    -   **`ActionContext` and `Verdict`**: When a check is made, the calling code provides an `ActionContext` struct, describing the source and destination of the data (e.g., URLs, incognito status). The `ChromeRulesService` evaluates this context against its `Rule` objects and returns a `Verdict`.
    -   **Verdict Enforcement**: The `Verdict` can be `Block`, `Warn`, `Report`, or `Allow`. The code that called the service is then responsible for enforcing this verdict (e.g., preventing the paste, showing a warning dialog, etc.). This decouples the rules engine from the enforcement points.

### 2.5 `encryption/`

-   **Purpose**: Provides a mechanism to encrypt the browser's on-disk HTTP cache, as enforced by enterprise policy. This protects sensitive data from enterprise sites that may be cached on a user's machine.
-   **Design**:
    -   **Mojo Service**: The core of the feature is the `CacheEncryptionProviderImpl`, which runs as a Mojo service in the privileged browser process. It implements the `network::mojom::CacheEncryptionProvider` interface.
    -   **Privilege Separation**: When the sandboxed network service needs to encrypt or decrypt a cache entry, it requests an `Encryptor` object via this Mojo interface.
    -   **OS Integration**: The `CacheEncryptionProviderImpl` does not perform encryption itself. Instead, it delegates the cryptographic operations to `os_crypt_async::OSCryptAsync`, which uses the underlying operating system's native encryption services (e.g., DPAPI on Windows, Keychain on macOS). This design prevents the sandboxed network process from ever having direct access to OS-level encryption keys.

### 2.6 `idle/`

-   **Purpose**: Implements the "Idle Timeout" feature for enterprises. It allows administrators to define a set of actions (e.g., close browser windows, clear data) that execute automatically after a user has been idle for a specified period.
-   **Design**:
    -   **`IdleService`**: The central `KeyedService` for this feature. It uses the `ui::IdlePollingService` to detect user inactivity. It is configured by the `IdleTimeout` and `IdleTimeoutActions` policies.
    -   **`ActionFactory`**: A singleton that translates the `IdleTimeoutActions` policy strings (e.g., "clear_cookies") into a priority queue of concrete `Action` objects.
    -   **Dialogs and Destructive Actions**: If a destructive action (like clearing data) is configured, the `ActionFactory` automatically prepends a `ShowDialogAction` to the queue. This action uses a `DialogManager` to show a 30-second warning dialog, giving the user a chance to cancel.
    -   **`ActionRunner`**: A dedicated class that executes the queue of actions in order of priority.

### 2.7 `identifiers/`

-   **Purpose**: Provides a stable, unique identifier for a browser profile, which is essential for enterprise management and reporting. This allows an admin to correlate activity with a specific profile across different sessions and devices.
-   **Design**:
    -   **Layering**: The core framework (e.g., `ProfileIdService`) is defined in `components/enterprise/browser/identifiers/`. This directory provides the browser-specific implementation, `ProfileIdDelegateImpl`.
    -   **Delegate Pattern**: The `ProfileIdDelegateImpl` implements the abstract `ProfileIdDelegate` interface. Its main job is to provide a device-level ID (`GetDeviceId()`) to the `ProfileIdService`.
    -   **Service Factory**: The `ProfileIdServiceFactory` constructs the service and injects the `ProfileIdDelegateImpl`, cleanly wiring the generic component to its browser-specific dependency.
    -   **Preset IDs**: The component also supports presetting a GUID for a new profile via the `PresetProfileManagementData` class, allowing for deterministic profile identification during setup.

### 2.8 `net/`

-   **Purpose**: This directory contains enterprise-specific features related to network requests and traffic management. *(Analysis Incomplete)*
-   **Presumed Design**: Based on file names, it likely contains logic for transparently attaching authentication headers to network requests destined for specific enterprise domains, and potentially other network-level interceptions.

### 2.9 `platform_auth/`

-   **Purpose**: Implements a cross-platform framework for enabling platform-level Single Sign-On (SSO). It allows Chrome to leverage built-in OS authentication mechanisms (e.g., Windows CloudAP, macOS Extensible SSO) to acquire authentication tokens for corporate Identity Providers (IdPs).
-   **Design**:
    -   **`PlatformAuthNavigationThrottle`**: This throttle intercepts navigations to IdP URLs that are pre-configured by enterprise policy.
    -   **`PlatformAuthProviderManager`**: This manager is the central entry point. It detects the current OS and selects the correct platform-specific provider.
    -   **`PlatformAuthProvider` Interface**: A generic interface implemented by platform-specific providers (`CloudApProviderWin`, `ExtensibleEnterpriseSsoProviderMac`). These providers handle the direct interaction with the OS APIs to fetch authentication tokens, which are then attached to the network request.

### 2.10 `profile_management/`

-   **Purpose**: This component handles enterprise features related to the management and branding of browser profiles.
-   **Design**: The main feature appears to be "profile branding," where an administrator can customize the new tab page and profile icon for managed profiles.
    - **`ProfileManagementService`**: A `KeyedService` that likely orchestrates profile branding.
    - **Policy Integration**: It is probably driven by policies that specify the background color, logo, and other visual elements.
    - **Difference from `signin/`**: While `signin/` is about the *creation* and *onboarding* of managed profiles, `profile_management/` is about managing the *look and feel* of those profiles after they have been created. *(Analysis Incomplete)*

### 2.11 `remote_commands/`

-   **Purpose**: Implements a system for the browser to receive and execute commands from a remote admin console (via the DM server).
-   **Design**:
    - **`RemoteCommandsService`**: This `KeyedService` listens for commands pushed from the DM server via the `CloudPolicyClient`.
    - **Command Fetching**: The `RemoteCommandJob` class defines the interface for a command, and the `RemoteCommandsFetcher` is responsible for retrieving them.
    - **Supported Commands**: The commands appear to be related to device state, such as fetching device information or triggering a remote device wipe. *(Analysis Incomplete)*

### 2.12 `reporting/`

-   **Purpose**: This component implements the architecture for gathering and uploading reports about the browser, profile, and device to an enterprise device management (DM) server. It is the core of our "Real-time Activity Monitoring" feature, but it is architected with two distinct pipelines.
-   **Design**:
    -   **Layering**: The core, cross-platform logic resides in `components/enterprise/browser/reporting/`. The browser-specific wiring, including integration with `Profile` and `CloudPolicyClient`, is in `chrome/browser/enterprise/reporting/`.
    -   **Batch Reporting Pipeline**: This is the primary mechanism for periodic, comprehensive reports.
        -   **`ReportScheduler`**: Orchestrates the timing of report generation and upload, based on enterprise policy.
        -   **`ReportGenerator`**: Gathers a wide array of data points (browser version, user info, machine info, profiles, policies, etc.) into a `BrowserReport` protobuf message.
        -   **`ReportUploader`**: Takes the generated report and uses a `CloudPolicyClient` to upload it to the DM server.
    -   **Real-time Reporting Pipeline**: This mechanism is for immediate, event-driven notifications.
        -   **`RealTimeReportController`**: Manages the real-time pipeline.
        -   It is used for reporting on security-sensitive events as they happen, such as a user trying to install a blocked extension.
    -   **`CloudProfileReportingService`**: A `KeyedService` that initializes and manages the reporting infrastructure for a given profile.

### 2.13 `signals/`

-   **Purpose**: This component is a **data provider** for other enterprise features, primarily the **Device Trust** connector. Its role is to collect signals about the device (OS, security state) and the user's profile to help determine if the current context is trustworthy.
-   **Design**:
    -   **Signal Collection**: The core logic resides in `components/device_signals/` and `chrome/browser/enterprise/signals/`. It contains various `*Fetcher` classes (`ClientCertificateFetcher`, `DeviceInfoFetcher`) responsible for gathering a specific piece of information.
    -   **Aggregation**: A `SignalsAggregator` collects the data from the various fetchers.
    -   **Service Consumer**: The primary consumer of these signals is the `DeviceTrustService` located in `chrome/browser/enterprise/connectors/device_trust/`. This service uses the signals to make a device trust attestation, which can then be used by other systems (e.g., to grant or deny access to a resource).
    -   **Filtering**: The `SignalsFilterer` class suggests that policies can control which signals are collected and shared, ensuring privacy and relevance.

### 2.14 `signin/`

-   **Purpose**: This component integrates enterprise-specific logic into Chromium's standard authentication flow. It is central to the "Identity-based Startup & Session" feature, enabling the creation of managed profiles and enforcing sign-in policies.
-   **Design**:
    -   **Interception**: The architecture is based on extending, not replacing, the core sign-in flow. The `OidcAuthenticationSigninInterceptor` class inherits from the generic `WebSigninInterceptor` to specifically handle web-based sign-ins from enterprise identity providers (like OIDC).
    -   **Onboarding Flow**: Upon intercepting a successful sign-in, the interceptor orchestrates the enterprise onboarding process:
        1.  It uses a `policy::CloudPolicyClient` to register the device with a management server.
        2.  It delegates to a `ManagedProfileCreationController`, which fetches "profile separation" policies to determine how to handle the new managed account.
        3.  The controller shows a UI disclaimer to the user and, based on policy and user consent, either creates a new, separate managed profile or converts the existing profile to be managed.
    -   **Policy Enforcement**:
        -   **`EnterpriseSigninService`**: This background service monitors the user's sign-in status and can trigger re-authentication if the state violates enterprise policy.
        -   **`ManagedProfileRequiredNavigationThrottle`**: This class can block navigations that require a managed profile and show an interstitial page, prompting the user to switch to the correct profile.
    -   **Relationship to Core Sign-in**: The component is a client of the core sign-in infrastructure, using `signin::IdentityManager` to query account information and the DICE framework (`DiceSignedInProfileCreator`) for profile creation.

### 2.15 `watermark/`

-   **Purpose**: Implements the visual watermarking feature for enterprise-managed browsers, intended to deter data leakage via screenshots or photographs.
-   **Design**:
    -   **Layering**: The core drawing logic is in `components/enterprise/watermarking/`, while the browser-specific view is in `chrome/browser/enterprise/watermark/`.
    -   **`WatermarkView`**: This `views::View` subclass is the primary UI component. It creates its own transparent, non-interactive `ui::Layer` that sits on top of the browser content.
    -   **Drawing Logic**: The view delegates the complex task of drawing to the `watermark::DrawWatermark` function. This function takes a `cc::PaintCanvas`, rotates it 45 degrees, and efficiently tiles a rendered text block in a staggered, brick-like pattern across the entire view.
    -   **Policy-Driven Style**: The visual style of the watermark (opacity, font size, colors) is configured via the `WatermarkStyle` enterprise policy. A `WatermarkStylePolicyHandler` reads this policy and writes the values into `PrefService`, which the `WatermarkView` then reads to style the canvas. The actual text for the watermark is likely derived from a separate policy.

---

## 3. `chrome/enterprise_companion/` Overview

-   **Purpose**: This directory contains a separate helper application or service that runs alongside the main Chrome browser. Its role is to perform privileged operations that the sandboxed browser process cannot.
-   **Design**: It communicates with the browser process via Native Messaging. This allows it to, for example, gather more detailed OS-level signals (like detailed security software state) or interact with other installed applications for security and management purposes, and then pass that information back to the browser. It acts as a "companion" to the browser to enhance its data-gathering and management capabilities in a secure way. *(Analysis Incomplete)*
